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

#include "orphaned_metadata.h"

#include <sqlite3.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <filesystem>
#include <iostream>
#include <stack>

OrphanedMetadataFix::OrphanedMetadataFix(
    std::filesystem::path root, std::filesystem::path path
) {
  root_path = root;
  obj_path = path;
}

void OrphanedMetadataFix::fix() {}

std::string OrphanedMetadataFix::to_string() const {
  std::string uuid = obj_path.string();
  return "orphaned metadata: " + uuid;
}

OrphanedMetadataCheck::OrphanedMetadataCheck(std::filesystem::path path) {
  root_path = path;
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

OrphanedMetadataCheck::~OrphanedMetadataCheck() {}

int OrphanedMetadataCheck::check() {
  int orphan_count = 0;
  std::string query = "SELECT uuid FROM objects WHERE uuid IS NOT NULL;";
  int rc = 0;
  sqlite3_stmt* stm;

  rc = metadata->prepare(query, &stm);
  if (rc != SQLITE_OK) {
    return rc;
  }

  rc = sqlite3_step(stm);
  while (rc == SQLITE_ROW && sqlite3_column_count(stm) > 0) {
    std::cout << "checking metadata: " << sqlite3_column_text(stm, 0)
              << std::endl;
    rc = sqlite3_step(stm);
  }
  sqlite3_finalize(stm);

  return orphan_count != 0 ? 1 : 0;
}
