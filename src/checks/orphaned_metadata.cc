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
  std::string query = "SELECT uuid FROM objects WHERE uuid IS NOT NULL;";
  int rc = 0;
  sqlite3_stmt* stm;
  const char* unused = 0;
}

OrphanedMetadataCheck::~OrphanedMetadataCheck() {}

int OrphanedMetadataCheck::check() {
  int orphan_count = 0;
  return orphan_count != 0 ? 1 : 0;
}
