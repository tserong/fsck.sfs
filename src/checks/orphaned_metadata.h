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
 * Orphaned Metadata Check
 * This check will iterate over all object metadata in the database and ensure
 * that the referenced objects can be found in the filesystem on disk. Should an
 * object or a version of an object not be found on disk, when the metadata
 * suggests it should be there, the fix will be to delete the metadata.
 */

#ifndef FSCK_SFS_SRC_CHECKS_ORPHANED_METADATA_H__
#define FSCK_SFS_SRC_CHECKS_ORPHANED_METADATA_H__

#include <filesystem>
#include <memory>

#include "checks.h"

class OrphanedMetadataFix : public Fix {
 private:
  std::filesystem::path obj_path;  // relative to root_path

  std::string to_string() const;

 public:
  OrphanedMetadataFix(
      const std::filesystem::path& root, const std::filesystem::path& object
  );
  operator std::string() const { return to_string(); };
  void fix();
};

class OrphanedMetadataCheck : public Check {
 protected:
  virtual bool do_check() override;

 public:
  OrphanedMetadataCheck(const std::filesystem::path& path)
      : Check("orphaned metadata", NONFATAL, path) {}
  virtual ~OrphanedMetadataCheck() override {}
};

#endif  // FSCK_SFS_SRC_CHECKS_ORPHANED_METADATA_H__
