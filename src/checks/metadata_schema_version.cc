/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
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

bool MetadataSchemaVersionCheck::do_check() {
  int version = 0;
  sqlite3_stmt* stm = metadata->prepare("PRAGMA user_version;");
  if (sqlite3_step(stm) == SQLITE_ROW && sqlite3_column_count(stm) > 0) {
    version = sqlite3_column_int(stm, 0);
  } else {
    const char* err = sqlite3_errmsg(metadata->handle);
    sqlite3_finalize(stm);
    throw std::runtime_error(err);
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
