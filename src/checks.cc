/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
*/

#include "checks.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "checks/metadata_integrity.h"
#include "checks/metadata_schema_version.h"
#include "checks/object_integrity.h"
#include "checks/orphaned_metadata.h"
#include "checks/orphaned_objects.h"

void Check::fix() {
  for (std::shared_ptr<Fix> fix : fixes) {
    fix->fix();
  }
}

void Check::show() {
  for (std::shared_ptr<Fix> fix : fixes) {
    // TODO: figure out how to handle embedded newlines (see comment in
    // MetadataIntegrityFix::to_string())
    std::cout << "  " << std::string(*fix) << std::endl;
  }
}

bool Check::check(Check::LogLevel log_level) {
  check_log_level = log_level;
  if (log_level > SILENT) {
    std::cout << "Checking " << check_name << "..." << std::endl;
  }
  return do_check();
}

void Check::log_verbose(const std::string& msg) const {
  if (check_log_level == VERBOSE) {
    std::cout << "  " << msg << std::endl;
  }
}

bool run_checks(
    const std::filesystem::path& path, Check::LogLevel log_level,
    bool should_fix
) {
  bool all_checks_passed = true;

  std::vector<std::shared_ptr<Check>> checks;
  checks.emplace_back(std::make_shared<MetadataIntegrityCheck>(path));
  checks.emplace_back(std::make_shared<MetadataSchemaVersionCheck>(path));
  checks.emplace_back(std::make_shared<OrphanedObjectsCheck>(path));
  checks.emplace_back(std::make_shared<OrphanedMetadataCheck>(path));
  checks.emplace_back(std::make_shared<ObjectIntegrityCheck>(path));

  for (std::shared_ptr<Check> check : checks) {
    bool this_check_passed = check->check(log_level);
    if (log_level > Check::LogLevel::SILENT) {
      check->show();
    }
    if (!this_check_passed) {
      all_checks_passed = false;
      if (check->is_fatal()) {
        // Don't do any more checks if this check failure is fatal.
        // TODO: integrate check->fix() here (try the fix, re-run the
        // check, reset the failure state) otherwise you're basically
        // screwed if there's a fatal failure.
        // Actually...  Should we try the fix first (before setting
        // all_checks_passed = false), then if the fix succeeds assume
        // the check succeeded?  Or if the fix succeeds, re-run the
        // check?  Or just list what fixes were applied/attempted and
        // still count the check as failed because it failed the first
        // time, then leave it up to the user to re-run fsck if they
        // want to verify everything is really cool after the fixes?
        break;
      }
    }
    if (should_fix) {
      check->fix();
    }
  }

  if (all_checks_passed && log_level > Check::LogLevel::SILENT) {
    std::cout << "All checks passed." << std::endl;
  }
  return all_checks_passed;
}
