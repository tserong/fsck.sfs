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

int run_checks(std::filesystem::path path, bool should_fix) {
  int has_problem = 0;

  std::vector<std::shared_ptr<Check>> checks;
  checks.emplace_back(std::make_shared<OrphanedObjectsCheck>(path));

  for (std::shared_ptr<Check> check : checks) {
    has_problem = check->check() == 0 ? has_problem : 1;
    check->show();
    if (should_fix) {
      check->fix();
    }
  }

  return has_problem;
}
