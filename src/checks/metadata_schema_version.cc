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

#include "metadata_schema_version.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

MetadataSchemaVersionFix::MetadataSchemaVersionFix(
    std::filesystem::path root, int version
) {
  root_path = root;
  schema_version = version;
}

void MetadataSchemaVersionFix::fix() {}

std::string MetadataSchemaVersionFix::to_string() const {
  return "Wrong metadata schema version: " + std::to_string(schema_version) +
         "; expected: " + std::to_string(EXPECTED_METADATA_SCHEMA_VERSION);
}

MetadataSchemaVersionCheck::MetadataSchemaVersionCheck(
    std::filesystem::path path
) {
  root_path = path;
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

MetadataSchemaVersionCheck::~MetadataSchemaVersionCheck() {}

int MetadataSchemaVersionCheck::check() {
  std::string query = "PRAGMA user_version;";
  int version = 0;
  int rc = 0;
  sqlite3_stmt* stm;
  rc = metadata->prepare(query, &stm);
  if (rc != SQLITE_OK) {
    return 1;
  }
  rc = sqlite3_step(stm);
  if (rc == SQLITE_ROW && sqlite3_column_count(stm) > 0) {
    version = sqlite3_column_int(stm, 0);
  } else {
    std::cout << "Error" << sqlite3_errmsg(metadata->handle) << std::endl;
  }
  sqlite3_finalize(stm);
  if (version != EXPECTED_METADATA_SCHEMA_VERSION) {
    fixes.emplace_back(
        std::make_shared<MetadataSchemaVersionFix>(root_path, version)
    );
  }
  return version == EXPECTED_METADATA_SCHEMA_VERSION ? 0 : 1;
}
