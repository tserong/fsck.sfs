/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
 */

#include "object_integrity.h"

#include <sqlite3.h>

#include <filesystem>
#include <iostream>

ObjectIntegrityFix::ObjectIntegrityFix(
    const std::filesystem::path& root, const std::filesystem::path& object,
    const std::string& _reason
)
    : Fix(root), obj_path(object), reason(_reason) {}

void ObjectIntegrityFix::fix() {}

std::string ObjectIntegrityFix::to_string() const {
  return "object data integrity check failed: " + obj_path.string() + "\n  " +
         reason;
}

bool ObjectIntegrityCheck::do_check() {
  int fail_count = 0;
  // TODO: is there any sense in implementing this as a separate check
  // class as I have it here?  Why not just slide it in as part of the
  // OrphanedMetadataCheck, given it's just an expanded form of the same
  // query, and the rest of the code is about the same...?
  std::string query =
      "SELECT object_id, id, checksum, size FROM versioned_objects "
      "WHERE object_id IS NOT NULL;";

  Statement stm(metadata->handle, query);
  int rc = sqlite3_step(stm);
  while (rc == SQLITE_ROW && sqlite3_column_count(stm) > 0) {
    std::string uuid{
        reinterpret_cast<const char*>(sqlite3_column_text(stm, 0))};
    std::string id{reinterpret_cast<const char*>(sqlite3_column_text(stm, 1))};
    log_verbose("Checking object " + id + " (uuid: " + uuid + ")");
    id.append(".v");
    std::string checksum{
        reinterpret_cast<const char*>(sqlite3_column_text(stm, 2))};
    std::uintmax_t object_size = sqlite3_column_int64(stm, 3);
    // first/second/fname logic lifted from sfs's UUIDPath class
    // *again* as we already did in OrphanedMetadataCheck
    // TODO: dedupe this
    std::filesystem::path first = uuid.substr(0, 2);
    std::filesystem::path second = uuid.substr(2, 2);
    std::filesystem::path fname = uuid.substr(4);
    std::filesystem::path obj_path = first / second / fname / id;

    // Have to check the file exists first (it won't exist if it's
    // orphaned metadata, but as all checks run independently of
    // each other, we have to re-check here - this is another argument
    // for folding the integrity check into the orphaned metadata check)
    if (std::filesystem::exists(root_path / obj_path) &&
        std::filesystem::is_regular_file(root_path / obj_path)) {
      std::uintmax_t file_size =
          std::filesystem::file_size(root_path / obj_path);
      if (file_size != object_size) {
        fixes.emplace_back(std::make_shared<ObjectIntegrityFix>(
            root_path, obj_path,
            std::string(
                "size mismatch (got " + std::to_string(file_size) +
                ", expected " + std::to_string(object_size) + ")"
            )
        ));
        fail_count++;
      }

      // TODO: implement checksum check
    }

    rc = sqlite3_step(stm);
  }

  return fail_count == 0;
}
