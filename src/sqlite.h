/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
 *
 * - - -
 *
 * C++ helper bindings for SQLite.
 */

#ifndef FSCK_SFS_SRC_SQLITE_H__
#define FSCK_SFS_SRC_SQLITE_H__

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

  int prepare(std::string, sqlite3_stmt**);

  int count_in_table(std::string, std::string);
  std::vector<std::string> select_from_table(std::string, std::string);
};

#endif  // FSCK_SFS_SRC_SQLITE_H__
