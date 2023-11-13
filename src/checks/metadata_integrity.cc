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

#include "metadata_integrity.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

MetadataIntegrityFix::MetadataIntegrityFix(
    const std::filesystem::path& path, const std::vector<std::string>& _errors
)
    : Fix(path), errors(_errors) {}

void MetadataIntegrityFix::fix() {}

std::string MetadataIntegrityFix::to_string() const {
  std::string msg("Database integrity check failed:");
  for (auto& s : errors) {
    // note: this indent needs to match the indent in Check::show()
    // TODO: figure out if we can drop the indent here and make Check::show()
    // handle it automatically when it sees newlines.
    msg += "\n  - " + s;
  }
  return msg;
}

MetadataIntegrityCheck::MetadataIntegrityCheck(const std::filesystem::path& path
)
    : Check("metadata integrity", FATAL, path) {
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

MetadataIntegrityCheck::~MetadataIntegrityCheck() {}

bool MetadataIntegrityCheck::do_check() {
  std::vector<std::string> errors;
  auto callback = [](void* arg, int num_columns, char** column_data, char**) {
    assert(num_columns == 1);
    // If this returns anything other than "ok", we'll end up with
    // information added to the errors vector, so will know something
    // is broken.  These are more fine grained than a completely trashed
    // database, for example:
    // - row 21 missing from index vobjs_object_id_idx
    // - non-unique entry in index versioned_object_objid_vid_unique
    if (std::strcmp(column_data[0], "ok") != 0) {
      static_cast<std::vector<std::string>*>(arg)->emplace_back(column_data[0]);
    }
    return 0;
  };

  // This will return up to 100 error rows by default.  For more details see
  // https://www.sqlite.org/pragma.html#pragma_integrity_check
  int rc = sqlite3_exec(
      metadata->handle, "PRAGMA integrity_check", callback,
      static_cast<void*>(&errors), NULL
  );

  if (rc != SQLITE_OK) {
    // This will happen if the file is _so_ trashed it doesn't even
    // look like an SQLite database.  Here you'll see things like:
    // - file is not a database
    // - database disk image is malformed
    errors.emplace_back(sqlite3_errstr(rc));
  }
  if (!errors.empty()) {
    fixes.emplace_back(std::make_shared<MetadataIntegrityFix>(root_path, errors)
    );
    return false;
  }
  return true;
}
