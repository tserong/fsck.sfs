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
    const std::filesystem::path& path, int version
)
    : Fix(path), schema_version(version) {}

void MetadataSchemaVersionFix::fix() {}

std::string MetadataSchemaVersionFix::to_string() const {
  return "Wrong metadata schema version: " + std::to_string(schema_version) +
         "; expected: " + std::to_string(EXPECTED_METADATA_SCHEMA_VERSION);
}

MetadataSchemaVersionCheck::MetadataSchemaVersionCheck(
    const std::filesystem::path& path
)
    : Check("metadata schema version", FATAL, path) {
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

MetadataSchemaVersionCheck::~MetadataSchemaVersionCheck() {}

bool MetadataSchemaVersionCheck::do_check() {
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
    // FIXME: this should probably raise an exception (at the
    // very least we shouldn't be printing anything directly here)
    std::cout << "Error" << sqlite3_errmsg(metadata->handle) << std::endl;
  }
  sqlite3_finalize(stm);
  log_verbose("Got schema version " + std::to_string(version));
  if (version != EXPECTED_METADATA_SCHEMA_VERSION) {
    fixes.emplace_back(
        std::make_shared<MetadataSchemaVersionFix>(root_path, version)
    );
  }
  return version == EXPECTED_METADATA_SCHEMA_VERSION;
}
