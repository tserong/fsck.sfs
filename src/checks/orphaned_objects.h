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
 *
 * - - -
 *
 * This implements a check for orphaned objects in an s3gw volume. An object is
 * orphaned, when there is no clean way to access it through the s3gw, because
 * it is not referenced in the metadata store.
 *
 * The standard fix to finding orphaned objects is to move them into a
 * "lost+found" directory at the root of the volume, where they can be retrieved
 * and reviewed by a system administrator.
*/

#ifndef FSCK_S3GW_SRC_CHECKS_ORPHANED_OBJECTS_H__
#define FSCK_S3GW_SRC_CHECKS_ORPHANED_OBJECTS_H__

#include <filesystem>
#include <memory>

#include "checks.h"
#include "sqlite.h"

class OrphanedObjectsFix : public Fix {
 private:
  std::filesystem::path root_path;
  std::filesystem::path obj_path;  // relative to root_path

  std::string to_string() const;

 public:
  OrphanedObjectsFix(std::filesystem::path, std::filesystem::path);
  operator std::string() const { return to_string(); };
  void fix();
};

class UnexpectedFileFix : public Fix {
 private:
  std::filesystem::path root_path;
  std::filesystem::path obj_path;  // relative to root_path

  std::string to_string() const;

 public:
  UnexpectedFileFix(std::filesystem::path, std::filesystem::path);
  operator std::string() const { return to_string(); };
  void fix();
};

class OrphanedObjectsCheck : public Check {
 private:
  std::filesystem::path root_path;
  std::unique_ptr<Database> metadata;

 public:
  OrphanedObjectsCheck(std::filesystem::path);
  virtual ~OrphanedObjectsCheck() override;
  virtual int check() override;
};

#endif  // FSCK_S3GW_SRC_CHECKS_ORPHANED_OBJECTS_H__
