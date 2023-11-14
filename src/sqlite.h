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
  const std::filesystem::path& db;

 public:
  sqlite3* handle;
  Database(const std::filesystem::path& _db);
  ~Database();

  int prepare(const std::string& query, sqlite3_stmt** stm) const;

  int count_in_table(const std::string& table, const std::string& condition)
      const;
  std::vector<std::string> select_from_table(
      const std::string& table, const std::string& column
  ) const;
};

#endif  // FSCK_SFS_SRC_SQLITE_H__
