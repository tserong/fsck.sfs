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

#ifndef FSCK_SFS_SRC_CHECKS_METADATA_INTEGRITY_H__
#define FSCK_SFS_SRC_CHECKS_METADATA_INTEGRITY_H__

#include <filesystem>
#include <memory>

#include "checks.h"
#include "sqlite.h"

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
 private:
  std::unique_ptr<Database> metadata;

 protected:
  virtual bool do_check() override;

 public:
  MetadataIntegrityCheck(const std::filesystem::path& path);
  virtual ~MetadataIntegrityCheck() override;
};

#endif  // FSCK_SFS_SRC_CHECKS_METADATA_INTEGRITY_H__
