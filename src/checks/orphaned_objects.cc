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

OrphanedObjectsFix::OrphanedObjectsFix(
    const std::filesystem::path& root, const std::filesystem::path& object
)
    : Fix(root), obj_path(object) {}

void OrphanedObjectsFix::fix() {
  // TODO: print a message of what was actually done when the fix was applied!
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
  // TODO: this stripping is wrong, because it prints:
  // orphaned object: 353e5262-d525-42bd-a43a-876a0938b4842.p at 35/3e/5262-d525-42bd-a43a-876a0938b484/2.p
  // (note how the 2.p is smooshed onto the end of the UUID?)
  boost::erase_all(oid, "/");
  return "orphaned object: " + oid + " at " + obj_path.string();
}

UnexpectedFileFix::UnexpectedFileFix(
    const std::filesystem::path& root, const std::filesystem::path& object
)
    : Fix(root), obj_path(object) {}

void UnexpectedFileFix::fix() {
  // TODO: do we really want to move unexpected files to lost+found?
}

std::string UnexpectedFileFix::to_string() const {
  return "Found unexpected mystery file: " + obj_path.string();
}

OrphanedObjectsCheck::OrphanedObjectsCheck(const std::filesystem::path& path)
    : Check("orphaned objects", NONFATAL, path) {
  metadata = std::make_unique<Database>(root_path / "s3gw.db");
}

OrphanedObjectsCheck::~OrphanedObjectsCheck() {}

bool OrphanedObjectsCheck::do_check() {
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
        std::filesystem::path rel =
            std::filesystem::relative(cwd / entry.path(), root_path);

        log_verbose("Checking file " + rel.string());

        std::filesystem::path uuid_path =
            std::filesystem::relative(cwd, root_path);
        std::string uuid = uuid_path.string();
        boost::erase_all(uuid, "/");

        std::string stem(entry.path().stem());
        auto name_is_numeric = std::all_of(stem.begin(), stem.end(), ::isdigit);
        if (name_is_numeric && entry.path().extension() == ".v") {
          // It's a versioned object
          if (metadata->count_in_table(
                  "versioned_objects",
                  "object_id=\"" + uuid + "\" AND id=" + stem
              ) == 0) {
            fixes.emplace_back(
                std::make_shared<OrphanedObjectsFix>(root_path, rel.string())
            );
            orphan_count++;
          }
        } else if (name_is_numeric && entry.path().extension() == ".p") {
          // It's a multipart part
          std::string query =
              "SELECT COUNT(multiparts_parts.id) FROM multiparts_parts, "
              "multiparts "
              "WHERE multiparts_parts.upload_id = multiparts.upload_id AND "
              "      multiparts_parts.id = " +
              stem +
              " AND "
              "      path_uuid = '" +
              uuid + "'";
          sqlite3_stmt* stm;
          if (metadata->prepare(query, &stm) == SQLITE_OK) {
            if (sqlite3_step(stm) == SQLITE_ROW &&
                sqlite3_column_count(stm) > 0) {
              int count = sqlite3_column_int(stm, 0);
              if (count == 0) {
                fixes.emplace_back(
                    // TODO: Consider making an OrphanedMultipartFix class.
                    // OrphanedOjectsFix works fine, but the messaging might
                    // be slightly misleading ("orphaned object: uuid-n ..."
                    // vs. what would be "orhpaned multipart part: ...")
                    std::make_shared<OrphanedObjectsFix>(
                        root_path, rel.string()
                    )
                );
                orphan_count++;
              }
            } else {
              std::cout << "This can't happen" << std::endl;
              // TODO: You sure about that bro?
              // FIXME: Throw an exception here rather than printing to stdout
            }
            sqlite3_finalize(stm);
          } else {
            std::cout << "This shouldn't happen" << std::endl;
            // TODO: What?  Seriously?  Do better with the error handling.
            // FIXME: Throw an exception here rather than printing to stdout
          }
        } else {
          // This is something else
          fixes.emplace_back(
              std::make_shared<UnexpectedFileFix>(root_path, rel.string())
          );
          orphan_count++;
        }
      }
    }
  }
  return orphan_count == 0;
}
