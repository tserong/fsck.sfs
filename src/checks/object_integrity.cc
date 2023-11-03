/*
 * Copyright 2023 SUSE, LLC.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "object_integrity.h"

#include <sqlite3.h>

#include <filesystem>

ObjectIntegrityFix::ObjectIntegrityFix(
    const std::filesystem::path& _root, const std::filesystem::path& _path,
    const std::string& _reason
)
    : root_path(_root), obj_path(_path), reason(_reason) {}

void ObjectIntegrityFix::fix() {}

std::string ObjectIntegrityFix::to_string() const {
  return "object data integrity check failed: " + obj_path.string() + "\n  " +
         reason;
}

ObjectIntegrityCheck::ObjectIntegrityCheck(std::filesystem::path path) {
  root_path = path;
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

ObjectIntegrityCheck::~ObjectIntegrityCheck() {}

int ObjectIntegrityCheck::check() {
  int fail_count = 0;
  // TODO: is there any sense in implementing this as a separate check
  // class as I have it here?  Why not just slide it in as part of the
  // OrphanedMetadataCheck, given it's just an expanded form of the same
  // query, and the rest of the code is about the same...?
  std::string query =
      "SELECT object_id, id, checksum, size FROM versioned_objects "
      "WHERE object_id IS NOT NULL;";
  int rc = 0;
  sqlite3_stmt* stm;

  rc = metadata->prepare(query, &stm);
  if (rc != SQLITE_OK) {
    return rc;
  }

  rc = sqlite3_step(stm);
  while (rc == SQLITE_ROW && sqlite3_column_count(stm) > 0) {
    std::string uuid{
        reinterpret_cast<const char*>(sqlite3_column_text(stm, 0))};
    std::string id{reinterpret_cast<const char*>(sqlite3_column_text(stm, 1))};
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
  sqlite3_finalize(stm);

  return fail_count != 0 ? 1 : 0;
}
