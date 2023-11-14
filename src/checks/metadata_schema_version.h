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
 * This is a simple check that ensures the metadata database contains a schema
 * in the expected version
*/

#ifndef FSCK_SFS_SRC_CHECKS_METADATA_SCHEMA_VERSION_H__
#define FSCK_SFS_SRC_CHECKS_METADATA_SCHEMA_VERSION_H__

#include <filesystem>
#include <memory>

#include "checks.h"

class MetadataSchemaVersionFix : public Fix {
 private:
  int schema_version;

  std::string to_string() const;

 public:
  MetadataSchemaVersionFix(const std::filesystem::path& path, int version);
  operator std::string() const { return to_string(); };
  void fix();
};

class MetadataSchemaVersionCheck : public Check {
 protected:
  virtual bool do_check() override;

 public:
  MetadataSchemaVersionCheck(const std::filesystem::path& path)
      : Check("metadata schema version", FATAL, path) {}
  virtual ~MetadataSchemaVersionCheck() override {}
};

const int EXPECTED_METADATA_SCHEMA_VERSION = 4;

#endif  // FSCK_SFS_SRC_CHECKS_METADATA_SCHEMA_VERSION_H__
