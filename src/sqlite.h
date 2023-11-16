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

class Statement {
 private:
  sqlite3_stmt* stmt;

 public:
  Statement() = delete;
  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;
  Statement(sqlite3* db, const std::string& query) : stmt(nullptr) {
    if (sqlite3_prepare_v2(db, query.c_str(), query.length(), &stmt, nullptr) !=
        SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db));
    }
  };
  virtual ~Statement() { sqlite3_finalize(stmt); };
  operator sqlite3_stmt*() { return stmt; }
};

class Database {
 private:
  const std::filesystem::path& db;

 public:
  sqlite3* handle;
  Database(const std::filesystem::path& _db);
  ~Database();

  int count_in_table(const std::string& table, const std::string& condition)
      const;
  //std::vector<std::string> select_from_table(
  //    const std::string& table, const std::string& column
  //) const;
};

#endif  // FSCK_SFS_SRC_SQLITE_H__
