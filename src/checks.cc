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
    Log::log("  " + std::string(*fix));
  }
}

bool Check::check() {
  Log::log("Checking " + check_name + "...");
  return do_check();
}

bool run_checks(const std::filesystem::path& path, bool should_fix) {
  Log::log("Checking SFS store in " + path.string());
  bool all_checks_passed = true;

  std::vector<std::shared_ptr<Check>> checks;
  checks.emplace_back(std::make_shared<MetadataIntegrityCheck>(path));
  checks.emplace_back(std::make_shared<MetadataSchemaVersionCheck>(path));
  checks.emplace_back(std::make_shared<OrphanedObjectsCheck>(path));
  checks.emplace_back(std::make_shared<OrphanedMetadataCheck>(path));
  checks.emplace_back(std::make_shared<ObjectIntegrityCheck>(path));

  for (std::shared_ptr<Check> check : checks) {
    bool this_check_passed = check->check();
    check->show();
    if (!this_check_passed) {
      if (should_fix) {
        // Try to fix the issue if possible
        // TODO: Consider adding a 'continue' here if we know the fix
        // has succeeded, so we ultimately return success rather than
        // failure if everything is fixed.
        check->fix();
      }
      all_checks_passed = false;
      if (check->is_fatal()) {
        // Don't do any more checks if this check failure is fatal.
        // The fix might not have worked, or might not be possible,
        // so it's not safe to proceed further.
        break;
      }
    }
  }

  if (all_checks_passed) {
    Log::log("All checks passed.");
  } else {
    Log::log("One or more checks failed.");
  }
  return all_checks_passed;
}
