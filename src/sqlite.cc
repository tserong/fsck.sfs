/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
 */

#include "sqlite.h"

#include <sqlite3.h>

#include <filesystem>
#include <iostream>

Database::Database(const std::filesystem::path& _db)
    : db(_db), handle(nullptr) {
  int rc = sqlite3_open(db.string().c_str(), &handle);
  if (rc != SQLITE_OK) {
    const char* err = sqlite3_errmsg(handle);
    sqlite3_close(handle);
    throw std::runtime_error(err);
  }
}

Database::~Database() {
  sqlite3_close(handle);
}

sqlite3_stmt* Database::prepare(const std::string& query) const {
  sqlite3_stmt* stm;
  const char* unused = 0;
  if (sqlite3_prepare(handle, query.c_str(), query.length(), &stm, &unused) !=
      SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(handle));
  }
  return stm;
}

/* Count in Table - Count number of rows in named table where the condition
 * is true. Translates directly into:
 *
 *   SELECT COUNT(*) FROM table WHERE condition ;
 */
int Database::count_in_table(
    const std::string& table, const std::string& condition
) const {
  std::string query =
      "SELECT COUNT(*) FROM " + table + " WHERE " + condition + ";";
  int count = 0;
  int rc = 0;
  sqlite3_stmt* stm;
  const char* unused = 0;
  rc = sqlite3_prepare(handle, query.c_str(), query.length(), &stm, &unused);
  if (rc != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(handle));
  }
  rc = sqlite3_step(stm);
  if (rc == SQLITE_ROW && sqlite3_column_count(stm) > 0) {
    count = sqlite3_column_int(stm, 0);
  }
  sqlite3_finalize(stm);
  return count;
}

/* Select from Table - Get all non-null entries of one column from a table.
 * Translates into:
 *
 *   SELECT column FROM table WHERE column IS NOT NULL ;
 */
std::vector<std::string> Database::select_from_table(
    const std::string& table, const std::string& column
) const {
  std::string query = "SELECT " + column + " FROM " + table + " WHERE " +
                      column + " IS NOT NULL;";
  int rc = 0;
  sqlite3_stmt* stm;
  const char* unused = 0;
  std::vector<std::string> result;
  rc = sqlite3_prepare(handle, query.c_str(), query.length(), &stm, &unused);
  if (rc != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(handle));
  }
  do {
    rc = sqlite3_step(stm);

  } while (rc == SQLITE_ROW);
  sqlite3_finalize(stm);
  return result;
}
