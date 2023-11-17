/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
 */

#include "orphaned_objects.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <filesystem>
#include <iostream>
#include <stack>

void OrphanedObjectsFix::fix() {
  try {
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
    // FIXME: Integrate with log level
    std::cout << "  Moved " << obj_path.string() << " to lost+found"
              << std::endl;

    // remove directories above if no object remains
    std::filesystem::path parent(root_path / obj_path.parent_path());
    while (parent != root_path && is_empty(parent)) {
      std::filesystem::remove(parent);
      parent = parent.parent_path();
    }
  } catch (std::filesystem::filesystem_error& ex) {
    // TODO: better error reporting?
    // TODO: integrate with log level
    std::cerr << "Error: " << ex.what() << std::endl;
  }
}

std::string OrphanedObjectsFix::to_string() const {
  std::string msg("Found ");
  switch (type) {
    case OBJECT:
      msg += "orphaned object";
      break;
    case MULTIPART:
      msg += "orphaned multipart part";
      break;
    case UNKNOWN:
      msg += "unexpected file";
      break;
  }
  msg += ": " + obj_path.string();
  return msg;
}

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
            fixes.emplace_back(std::make_shared<OrphanedObjectsFix>(
                OrphanedObjectsFix::OBJECT, root_path, rel.string()
            ));
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
          Statement stm(metadata->handle, query);
          if (sqlite3_step(stm) == SQLITE_ROW &&
              sqlite3_column_count(stm) > 0) {
            int count = sqlite3_column_int(stm, 0);
            if (count == 0) {
              fixes.emplace_back(std::make_shared<OrphanedObjectsFix>(
                  OrphanedObjectsFix::MULTIPART, root_path, rel.string()
              ));
              orphan_count++;
            }
          } else {
            // This can't happen ("SELECT COUNT(...)" is _always_ going
            // to give us one row with one column...)
            throw std::runtime_error(sqlite3_errmsg(metadata->handle));
          }
        } else {
          // It's something else (neither versioned object nor multipart part).
          // Note that this once picked up a ".m" file, which is a combined
          // multipart upload temp file, prior to it being moved to the final
          // object.  No idea how I managed to hit that - it should be really
          // difficult...
          fixes.emplace_back(std::make_shared<OrphanedObjectsFix>(
              OrphanedObjectsFix::UNKNOWN, root_path, rel.string()
          ));
          orphan_count++;
        }
      }
    }
  }
  return orphan_count == 0;
}
