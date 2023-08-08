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
 * C++ helper bindings for SQLite.
 */

#ifndef FSCK_S3GW_SRC_SQLITE_H__
#define FSCK_S3GW_SRC_SQLITE_H__

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <vector>

class Database {
 private:
  std::filesystem::path db;

 public:
  sqlite3* handle;
  Database(std::filesystem::path);
  ~Database();

  bool check_integrity();

  int prepare(std::string, sqlite3_stmt**);

  int count_in_table(std::string, std::string);
  std::vector<std::string> select_from_table(std::string, std::string);
};

#endif  // FSCK_S3GW_SRC_SQLITE_H__
