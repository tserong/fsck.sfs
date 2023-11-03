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
*/

#include "checks.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

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
    std::cout << std::string(*fix) << std::endl;
  }
}

bool run_checks(const std::filesystem::path& path, bool should_fix) {
  bool all_checks_passed = true;

  std::vector<std::shared_ptr<Check>> checks;
  checks.emplace_back(std::make_shared<MetadataSchemaVersionCheck>(path));
  checks.emplace_back(std::make_shared<OrphanedObjectsCheck>(path));
  checks.emplace_back(std::make_shared<OrphanedMetadataCheck>(path));
  checks.emplace_back(std::make_shared<ObjectIntegrityCheck>(path));

  for (std::shared_ptr<Check> check : checks) {
    bool this_check_passed = check->check();
    check->show();
    if (!this_check_passed) {
      all_checks_passed = false;
      if (check->is_fatal()) {
        // Don't do any more checks if this check failure is fatal.
        // TODO: integrate check->fix() here (try the fix, re-run the
        // check, reset the failure state) otherwise you're basically
        // screwed if there's a fatal failure.
        break;
      }
    }
    if (should_fix) {
      check->fix();
    }
  }

  if (all_checks_passed) {
    std::cout << "Everything's cool." << std::endl;
  }
  return all_checks_passed;
}
