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

#ifndef FSCK_SFS_SRC_CHECKS_H__
#define FSCK_SFS_SRC_CHECKS_H__

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

/* Fix - This is an abstract datatype representing an executable action to fix
 * an incosistency in the filesystem or metadata database.
 */
class Fix {
 protected:
  const std::filesystem::path& root_path;

 public:
  Fix(const std::filesystem::path& path) : root_path(path) {}
  virtual ~Fix(){};
  virtual operator std::string() const = 0;
  virtual void fix() = 0;
};

/* Check - This is an abstract datatype representing exectutable action that
 * examines the given filesystem for a specific defect and products a list of
 * fixes. This can then be either acted upon to fix the filesystem or just be
 * reported to the user.
 */
class Check {
 protected:
  std::vector<std::shared_ptr<Fix>> fixes;
  const std::string check_name;
  enum Fatality { FATAL, NONFATAL } fatality;
  const std::filesystem::path& root_path;
  virtual bool do_check() = 0;
  void log_verbose(const std::string& msg) const;

 public:
  Check(const std::string& name, Fatality f, const std::filesystem::path& path)
      : check_name(name), fatality(f), root_path(path) {}
  virtual ~Check(){};
  enum LogLevel { SILENT, NORMAL, VERBOSE };
  bool check(LogLevel log_level = NORMAL);
  bool is_fatal() { return fatality == FATAL; }
  void fix();
  void show();

 private:
  // Transient, only valid inside check() so that log_verbose() works
  LogLevel check_log_level;
};

bool run_checks(
    const std::filesystem::path& path, Check::LogLevel log_level,
    bool should_fix
);

#endif  // FSCK_SFS_SRC_CHECKS_H__
