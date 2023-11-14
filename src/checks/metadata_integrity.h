/*
 * Copyright 2023 SUSE, LLC.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file LICENSE.
 */

#ifndef FSCK_SFS_SRC_CHECKS_METADATA_INTEGRITY_H__
#define FSCK_SFS_SRC_CHECKS_METADATA_INTEGRITY_H__

#include <filesystem>
#include <memory>

#include "checks.h"

class MetadataIntegrityFix : public Fix {
 private:
  std::string to_string() const;
  std::vector<std::string> errors;

 public:
  MetadataIntegrityFix(
      const std::filesystem::path& path, const std::vector<std::string>& _errors
  );
  operator std::string() const { return to_string(); };
  void fix();
};

class MetadataIntegrityCheck : public Check {
 protected:
  virtual bool do_check() override;

 public:
  MetadataIntegrityCheck(const std::filesystem::path& path)
      : Check("metadata integrity", FATAL, path) {}
  virtual ~MetadataIntegrityCheck() override{};
};

#endif  // FSCK_SFS_SRC_CHECKS_METADATA_INTEGRITY_H__
