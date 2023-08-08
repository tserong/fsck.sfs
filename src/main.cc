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
 *
 * - - -
 *
 * fsck.s3gw
 *
 * This program is used to check the filesystem backing the s3gw for consistency
 * and if needed repair any fixable problems found. It is meant to be run
 * manually by an administrator to recover from an inconsistent state and during
 * testing to ensure the s3gw terminates with the filesystem in a consistent
 * state even in the event of unrecoverable errors.
*/

#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>

#include "checks.h"
#include "sqlite.h"

#define FSCK_ASSERT(condition, message) \
  if (!condition) {                     \
    std::cerr << message << std::endl;  \
    return 1;                           \
  }

int main(int argc, char* argv[]) {
  boost::program_options::options_description desc("Allowed Options");
  desc.add_options()("help,h", "print this help text")(
      "fix,F", "fix any inconsistencies found"
  )("ignore-uninitialized,I",
    "don't return an error if the volume is uninitialized")(
      "path,p", boost::program_options::value<std::string>(), "path to check"
  )("quiet,q", "run silently")("verbose,v", "more verbose output");

  boost::program_options::positional_options_description p;
  p.add("path", -1);

  boost::program_options::variables_map options_map;
  boost::program_options::command_line_parser parser(argc, argv);
  boost::program_options::store(
      parser.options(desc).positional(p).run(), options_map
  );
  boost::program_options::notify(options_map);

  if (options_map.count("help")) {
    std::cout << desc << std::endl;
  }

  FSCK_ASSERT(options_map.count("path"), "Must supply path to check");

  std::string path_str(options_map["path"].as<std::string>());
  std::filesystem::path path_root(path_str);
  std::filesystem::path path_database = path_root / "s3gw.db";
  std::filesystem::path path_database_shm = path_root / "s3gw.db_tmp-shm";
  std::filesystem::path path_database_wal = path_root / "s3gw.db-wal";

  FSCK_ASSERT(std::filesystem::exists(path_root), "Path not found");
  FSCK_ASSERT(
      std::filesystem::is_directory(path_root), "Path must be a directory"
  );

  if (options_map.count("ignore-uninitialized") > 0) {
    if (!std::filesystem::exists(path_database)) {
      return 0;
    }
  } else {
    FSCK_ASSERT(
        std::filesystem::exists(path_database),
        "Metadata database not found (is this an s3gw volume?)"
    );
    FSCK_ASSERT(
        std::filesystem::is_regular_file(path_database),
        "Metadata database is not a regular file"
    );
    Database db(path_database);
    FSCK_ASSERT(db.check_integrity(),
        "Database integrity check failed"
    );
  }

  return run_checks(path_root, options_map.count("fix") > 0);
}
