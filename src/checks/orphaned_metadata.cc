/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
 */

#include "orphaned_metadata.h"

#include <sqlite3.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <filesystem>
#include <iostream>
#include <stack>

OrphanedMetadataFix::OrphanedMetadataFix(
    const std::filesystem::path& root, const std::filesystem::path& object
)
    : Fix(root), obj_path(object) {}

void OrphanedMetadataFix::fix() {}

std::string OrphanedMetadataFix::to_string() const {
  return "orphaned metadata: " + obj_path.string();
}

bool OrphanedMetadataCheck::do_check() {
  int orphan_count = 0;
  // TODO: Should we do a join here with the objects table in order
  // to get bucket id and object name for display purposes if something
  // is broken?
  std::string query =
      "SELECT object_id, id FROM versioned_objects WHERE object_id IS NOT "
      "NULL;";
  Statement stm(metadata->handle, query);

  int rc = sqlite3_step(stm);
  while (rc == SQLITE_ROW && sqlite3_column_count(stm) > 0) {
    std::string uuid{
        reinterpret_cast<const char*>(sqlite3_column_text(stm, 0))};
    std::string id{reinterpret_cast<const char*>(sqlite3_column_text(stm, 1))};
    log_verbose("Checking object " + id + " (uuid: " + uuid + ")");
    id.append(".v");
    // first/second/fname logic lifted from sfs's UUIDPath class
    std::filesystem::path first = uuid.substr(0, 2);
    std::filesystem::path second = uuid.substr(2, 2);
    std::filesystem::path fname = uuid.substr(4);
    std::filesystem::path obj_path = first / second / fname / id;
    if (!(std::filesystem::exists(root_path / obj_path) &&
          std::filesystem::is_regular_file(root_path / obj_path))) {
      fixes.emplace_back(
          std::make_shared<OrphanedMetadataFix>(root_path, obj_path)
      );
      orphan_count++;
    }
    rc = sqlite3_step(stm);
  }

  return orphan_count == 0;
}
