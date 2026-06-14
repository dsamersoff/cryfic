/*
 * Copyright © 2026 Dmitry Samersoff, all rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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

#ifndef _CRYFIC_HPP_
#define _CRYFIC_HPP_

#include <string>

#include <stddef.h>
#include <stdint.h>

#include "cryfic-db.hpp"

/* Versioning setup */
#include <openssl/opensslv.h>

// The version compiled with OpenSSL 1.0.x could be used for debugging purpose only. Don't use it in production.
#if OPENSSL_VERSION_NUMBER < 0x10101000L
#error "openssl 1.1.1 or openssl 3.x.x must be used to compile this program"
#endif

/* optional git version, set by build system */
#ifndef GIT_VERSION
#define GIT_VERSION __DATE__
#endif

#define CRYFIC_VERSION 0x00202901
#define CRYFIC_SCHEMA_VERSION 0x100

#define CRYFIC_DEFAULT_DBNAME "cryfic.sqlite3"
#define CRYFIC_DEFAULT_LOG_LEVEL LOG_DEBUG
#define CRYFIC_DEFAULT_HASH_ALGO "SHA256"

/**
 * @brief Environment var to set runtime options.
 *
 * e.g.
 *   CRYFIC_OPTIONS="verbose,stderr,no-syslog"
 */
#define CRYFIC_OPTIONS_VAR "CRYFIC_OPTIONS"

/**
 * @brief Name and path of configuration file.
 *
 * e.g.
 *    syslog=1
 */
#define CRYFIC_CONFIG_FILE_NAME "cryfic.conf"
#define CRYFIC_SYS_CONFIG_FILE "/etc/" CRYFIC_CONFIG_FILE_NAME

/**
 * @brief Initialize crypto engines and options from all available option sources.
 *
 * Perform required initialization for openssl libraries, also set library options.
 * Options source is processed in the next order: system config file, user config file, env var.
 * i.e. env var overrides config file.
 *
 * @param filename - name of config file to parse
 * @return Pointer to internal options structure
 */

void cryfic_init(const std::string& configfile, const std::string& dbname);

/**
 * @brief Go through filesystem from root_dir and add all found files
 *        to database. Optionally calculate it's hash
 *
 */
class CryficIndex : public CryficDB {
   private:
      void add_file(const std::string& filename);

   public:
      CryficIndex(const std::string& dbpath):CryficDB(dbpath) {}
      int run(const std::string& root_dir);
};

/**
 * @brief Go through the database and update hash of all found files
 *
 */
class CryficUpdate : public CryficDB {
   private:
      int process_row(int64_t r_id, const char *r_filename, FileStatus r_status, const char *r_hash_algo, const char *r_hash_hex);
   public:
      CryficUpdate(const std::string& dbpath):CryficDB(dbpath) {}
      int run();
};

/**
 * @brief Go through the database and verify stored hash of all found files
 *
 */
class CryficVerify : public CryficDB {
   private:
      int process_row(int64_t r_id, const char *r_filename, FileStatus r_status, const char *r_hash_algo, const char *r_hash_hex);
   public:
      CryficVerify(const std::string& dbpath):CryficDB(dbpath) {}
      int run();
};

#endif // _CRYFIC_HPP_
