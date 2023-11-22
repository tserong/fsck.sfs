/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
*/

#ifndef FSCK_SFS_SRC_CHECKS_H__
#define FSCK_SFS_SRC_CHECKS_H__

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "sqlite.h"

constexpr std::string_view DB_FILENAME = "sfs.db";

// TODO: this thing could conceivably keep track of an indent level,
// saving us from adding leading spaces in various places.
struct Log {
  enum Level { SILENT, NORMAL, VERBOSE };
  inline static Level level = NORMAL;
  static void log(const std::string& msg) {
    if (level > SILENT) {
      std::cout << msg << std::endl;
    }
  }
  static void log_verbose(const std::string& msg) {
    if (level == VERBOSE) {
      std::cout << "  " << msg << std::endl;
    }
  }
};

/* Fix - This is an abstract datatype representing an executable action to fix
 * an incosistency in the filesystem or metadata database.
 */
class Fix {
 protected:
  const std::filesystem::path& root_path;

 public:
  Fix(const std::filesystem::path& path) : root_path(path) {}
  virtual ~Fix(){};
  virtual operator std::string() const = 0;
  virtual void fix() = 0;
};

/* Check - This is an abstract datatype representing exectutable action that
 * examines the given filesystem for a specific defect and products a list of
 * fixes. This can then be either acted upon to fix the filesystem or just be
 * reported to the user.
 */
class Check {
 protected:
  std::vector<std::shared_ptr<Fix>> fixes;
  const std::string check_name;
  enum Fatality { FATAL, NONFATAL } fatality;
  const std::filesystem::path& root_path;
  std::unique_ptr<Database> metadata;
  virtual bool do_check() = 0;

 public:
  Check(const std::string& name, Fatality f, const std::filesystem::path& path)
      : check_name(name),
        fatality(f),
        root_path(path),
        metadata(std::make_unique<Database>(path / DB_FILENAME)) {}
  virtual ~Check(){};
  bool check();
  bool is_fatal() { return fatality == FATAL; }
  void fix();
  void show();
};

bool run_checks(const std::filesystem::path& path, bool should_fix);

#endif  // FSCK_SFS_SRC_CHECKS_H__
