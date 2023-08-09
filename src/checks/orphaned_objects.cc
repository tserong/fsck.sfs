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

#include "orphaned_objects.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <filesystem>
#include <iostream>
#include <stack>
#include <regex>

OrphanedObjectsFix::OrphanedObjectsFix(
    std::filesystem::path root, std::filesystem::path path
) {
  root_path = root;
  obj_path = path;
}

void OrphanedObjectsFix::fix() {
  if (!std::filesystem::exists(
          root_path / "lost+found" / obj_path.parent_path()
      )) {
    std::filesystem::create_directories(
        root_path / "lost+found" / obj_path.parent_path()
    );
  }

  std::filesystem::rename(
      root_path / obj_path, root_path / "lost+found" / obj_path
  );

  // remove directories above if no object remains
  if (std::filesystem::is_empty(root_path / obj_path.parent_path())) {
    std::filesystem::remove(root_path / obj_path.parent_path());
  }

  if (std::filesystem::is_empty(
          root_path / obj_path.parent_path().parent_path()
      )) {
    std::filesystem::remove(root_path / obj_path.parent_path().parent_path());
  }
}

std::string OrphanedObjectsFix::to_string() const {
  std::string oid = obj_path.string();
  boost::erase_all(oid, "/");
  return "orphaned object: " + oid + " at " + obj_path.string();
}

OrphanedObjectsCheck::OrphanedObjectsCheck(std::filesystem::path path) {
  root_path = path;
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

OrphanedObjectsCheck::~OrphanedObjectsCheck() {}

int OrphanedObjectsCheck::check() {
  int orphan_count = 0;
  std::stack<std::filesystem::path> stack;

  for (auto& entry : std::filesystem::directory_iterator{root_path}) {
    // ignore lost+found
    if (entry.path().filename().string().compare("lost+found") == 0) {
      continue;
    }

    if (std::filesystem::is_directory(entry.path())) {
      stack.push(entry.path());
    }
  }

  while (!stack.empty()) {
    std::filesystem::path cwd = stack.top();
    stack.pop();

    for (auto& entry : std::filesystem::directory_iterator{cwd}) {
      if (std::filesystem::is_directory(entry.path())) {
        stack.push(entry.path());
      } else {
        std::string filename(entry.path().filename());
        std::smatch match;
        if (std::regex_match(filename, match, std::regex("^[0-9]+$"))) {
          // It's a versioned object (their names are just integers)
          // TODO: verify each version isn't orphaned
          std::filesystem::path rel =
              std::filesystem::relative(cwd / entry.path(), root_path);
          std::filesystem::path uuid_path =
              std::filesystem::relative(cwd, root_path);
          std::string uuid = uuid_path.string();
          boost::erase_all(uuid, "/");
          if (metadata->count_in_table("objects", "uuid=\"" + uuid + "\"") == 0) {
            fixes.emplace_back(
                std::make_shared<OrphanedObjectsFix>(root_path, rel.string())
            );
            orphan_count++;
          }
        } else if (std::regex_match(filename, match,
                   std::regex("^(([[:xdigit:]]{4}-){4}[[:xdigit:]]{12})-([0-9]+)$"))) {
          // It's a multipart part.  Their names are "$uuid_tail-$part_number",
          // e.g.: "6c78-0c22-4c51-9a2b-4284724edd64-1"
          // match[1] == uuid tail
          // match[3] == multipart part number
          // TODO: Implement orphaned multipart Fix class
        } else {
          // This is something else
          // TODO: Implement this as a Fix class
          std::cout << "Found unexpected mystery file: " << entry.path().string() << std::endl;
          orphan_count++;
        }
      }
    }
  }
  return orphan_count != 0 ? 1 : 0;
}
