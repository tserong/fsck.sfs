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
 * This implements a check for orphaned objects in an s3gw volume. An object is
 * orphaned, when there is no clean way to access it through the s3gw, because
 * it is not referenced in the metadata store.
 *
 * The standard fix to finding orphaned objects is to move them into a
 * "lost+found" directory at the root of the volume, where they can be retrieved
 * and reviewed by a system administrator.
*/

#ifndef FSCK_SFS_SRC_CHECKS_ORPHANED_OBJECTS_H__
#define FSCK_SFS_SRC_CHECKS_ORPHANED_OBJECTS_H__

#include <filesystem>
#include <memory>

#include "checks.h"

class OrphanedObjectsFix : public Fix {
 private:
  std::filesystem::path obj_path;  // relative to root_path

  std::string to_string() const;

 public:
  OrphanedObjectsFix(
      const std::filesystem::path& root, const std::filesystem::path& object
  );
  operator std::string() const { return to_string(); };
  void fix();
};

class UnexpectedFileFix : public Fix {
 private:
  std::filesystem::path obj_path;  // relative to root_path

  std::string to_string() const;

 public:
  UnexpectedFileFix(
      const std::filesystem::path& root, const std::filesystem::path& object
  );
  operator std::string() const { return to_string(); };
  void fix();
};

class OrphanedObjectsCheck : public Check {
 protected:
  virtual bool do_check() override;

 public:
  OrphanedObjectsCheck(const std::filesystem::path& path)
      : Check("orphaned objects", NONFATAL, path) {}
  virtual ~OrphanedObjectsCheck() override {}
};

#endif  // FSCK_SFS_SRC_CHECKS_ORPHANED_OBJECTS_H__
