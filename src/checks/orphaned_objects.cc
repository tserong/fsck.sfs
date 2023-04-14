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

#include <algorithm>
#include <iostream>
#include <stack>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include "orphaned_objects.h"


OrphanedObjectsFix::OrphanedObjectsFix(
    boost::filesystem::path root,
    boost::filesystem::path path){
  root_path = root;
  obj_path = path;
}

void OrphanedObjectsFix::fix() {
  if (!boost::filesystem::exists(root_path / "lost+found")) {
    boost::filesystem::create_directory(root_path / "lost+found");
  }

  boost::filesystem::rename(
      root_path / *obj_path.begin(),
      root_path / "lost+found" / *obj_path.begin());
}

std::string OrphanedObjectsFix::to_string() const {
  std::string oid = obj_path.string();
  boost::erase_all(oid, "/");
  return "orphaned object: " + oid + " at " + obj_path.string();
}


OrphanedObjectsCheck::OrphanedObjectsCheck(boost::filesystem::path path) {
  root_path = path;
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

OrphanedObjectsCheck::~OrphanedObjectsCheck() {
}

int OrphanedObjectsCheck::check() {
  int orphan_count = 0;
  std::stack<boost::filesystem::path> stack;

  for (auto& entry : boost::make_iterator_range(
                      boost::filesystem::directory_iterator(root_path), {})) {
    // ignore lost+found
    if (entry.path().filename().string().compare("lost+found") == 0) {
      continue;
    }

    if (boost::filesystem::is_directory(entry.path())) {
      stack.push(entry.path());
    }
  }

  while(!stack.empty()) {
    boost::filesystem::path cwd = stack.top();
    stack.pop();

    for (auto& entry : boost::make_iterator_range(
                        boost::filesystem::directory_iterator(cwd), {})) {
      if (boost::filesystem::is_directory(entry.path())) {
        stack.push(entry.path());
      } else {
        boost::filesystem::path rel = boost::filesystem::relative(cwd, root_path);
        std::string uuid = rel.string();
        boost::erase_all(uuid, "/");
        if (metadata->count_in_table("objects", "object_id = \"" + uuid + "\"") == 0) {
          fixes.emplace_back(std::make_shared<OrphanedObjectsFix>(root_path, rel.string()));
          orphan_count++;
        }
      }
    }
  }
  return orphan_count != 0 ? 1 : 0;
}
