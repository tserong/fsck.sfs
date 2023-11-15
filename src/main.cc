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
 * fsck.sfs
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
  boost::program_options::variables_map options_map;
  try {
    boost::program_options::options_description desc("Allowed Options");
    desc.add_options()("help,h", "print this help text")(
        "fix,F", "fix any inconsistencies found"
    )("ignore-uninitialized,I",
      "don't return an error if the volume is uninitialized")(
        "path,p", boost::program_options::value<std::string>(), "path to check"
    )("quiet,q", "run silently")("verbose,v", "more verbose output");
    // TODO: it's currently possible to specify both quiet and
    // verbose at the same time.  This is a bit ridiculous.

    boost::program_options::positional_options_description p;
    p.add("path", -1);

    boost::program_options::command_line_parser parser(argc, argv);
    boost::program_options::store(
        parser.options(desc).positional(p).run(), options_map
    );
    boost::program_options::notify(options_map);

    if (options_map.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }
  } catch (const boost::program_options::error& ex) {
    // This will happen if you try to invoke with an unrecognised option
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  FSCK_ASSERT(options_map.count("path"), "Must supply path to check");

  std::string path_str(options_map["path"].as<std::string>());
  std::filesystem::path path_root(path_str);
  std::filesystem::path path_database = path_root / DB_FILENAME;

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
        "Metadata database not found (is this an sfs volume?)"
    );
    FSCK_ASSERT(
        std::filesystem::is_regular_file(path_database),
        "Metadata database is not a regular file"
    );
  }

  Check::LogLevel log_level = Check::LogLevel::NORMAL;
  if (options_map.count("quiet") > 0) {
    // TODO: fix discrepancy between terms "quiet" and "silent"?
    log_level = Check::LogLevel::SILENT;
  }
  if (options_map.count("verbose") > 0) {
    log_level = Check::LogLevel::VERBOSE;
  }

  return run_checks(path_root, log_level, options_map.count("fix") > 0) ? 0 : 1;
}
