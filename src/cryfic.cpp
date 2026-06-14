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

#include <filesystem>
#include <string>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

#include "cryfic.hpp"
#include "cryfic-logging.hpp"
#include "cryfic-db.hpp"
#include "cryfic-hash.hpp"

void cryfic_init(const std::string& configfile, const std::string& dbname) {
    CryficOptions& opts = CryficOptions::instance();

    // Initialize crypto engines
    OPENSSL_add_all_algorithms_conf();

    // We need to postpone diagnostic message to the end of options processing
    // to respect verbosity and output settings.
    int options_source = 0;

    // Hardcoded defaults
    // see CryficOptions ctor

    // Notice: If the config file change the verbosity subsequent logging will be performed at the config verbosity level
    // Try to read system config
    if (access(CRYFIC_SYS_CONFIG_FILE, R_OK) != -1) {
        opts.loadFromFile(CRYFIC_SYS_CONFIG_FILE);
        options_source |= 0x2;
    }

    // User config file overrides defaults and system config
    if (! configfile.empty()) {
        if (access(_S(configfile), R_OK) != -1) {
            opts.loadFromFile(configfile);
            options_source |= 0x4;
        }
    }

    const char *env_var = CRYFIC_OPTIONS_VAR;
    const char *env = getenv(env_var);
    if (env != NULL) {
        opts.loadFromEnv(env);
        options_source |= 0x8;
    }

    // Adjust dependent options
    opts.updateDependentOptions();

    if (opts.use_syslog()) {
       openlog("cryfic", LOG_PID, LOG_AUTH);
    }

    // Now we can log
    BasicLogger log = BasicLogger::instance();
    // Init database
    if (access(_S(dbname), F_OK) == -1) {
        // TODO: Check that existing schema is correct
        // TODO: Handle schema versioning
        CryficDB::db_create(dbname);
    }

    // log all pending messages
    log.noise("Compiled with openssl '%s' (%x)", OPENSSL_VERSION_TEXT, OPENSSL_VERSION_NUMBER);

    // Print options related diagnostics
    if (options_source & 0x1) {
        log.notice("Processed init arguments");
    }

    if (options_source & 0x2) {
        log.notice("Processed system config file '%s'", CRYFIC_SYS_CONFIG_FILE);
    }

    if (options_source & 0x4) {
        log.notice("Processed user config file '%s'", _S(configfile));
    }

    if (options_source & 0x8) {
        log.notice("Processed env %s=%s", env_var, env);
    }
}

// =================================================================================================
void CryficIndex::add_file(const std::string& filename) {
    std::string hash_algo;
    const char *hash_hex = nullptr;
    FileStatus status = FileStatus::New;
    int res = 0;

    if (filename == dbpath()) {
        log_.warn("database %s is inside protected dir, skipped", _S(dbpath_));
        return;
    }

    if (opts_.update()) {
        Hasher hs = Hasher::getHasher(opts_.hash_algo());

        int res = hs.hash_file(filename);
        if (res != 0) {
           log_.err("Error hashing (%s) ... %s", _S(hs.hash_algo()), _S(filename));
        }
        status = FileStatus::Hashed;
        hash_algo = hs.hash_algo();
        hash_hex = hs.hash_hex();

        res = db_add_file(filename, status, hash_algo, hash_hex);
        if (res == 0) {
            // Successfully inserted
            log_.notice("Inserted (HASHED, %s) ... %s", _S(hs.hash_algo()), _S(filename));
        } else {
            // File already exists - updating hash and status
            log_.notice("Updated (HASHED, %s) ... %s", _S(hs.hash_algo()), _S(filename));
            db_update_hash(filename, FileStatus::Hashed, hs.hash_algo(), hs.hash_hex());
        }
    }
    else {
        res = db_add_file(filename, status, hash_algo, hash_hex);
        if (res == 0) {
            // Successfully inserted
            log_.notice("Inserted (NEW) ... %s", _S(filename));
        }
        else {
            log_.noise("Skipped existing ... %s", _S(filename));
        }
    }
}

int CryficIndex::run(const std::string& root_dir) {
    if (opts_.purge()) {
        // Purge files marked as MISSING on the previous run
        log_.notice("Purging files with status ERROR (unreadable or missing) ... ");
        db_purge(FileStatus::Error);
    }

    // Scan directory recursively and add all found files to db
    std::error_code ec;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root_dir, ec)) {
        if (ec) {
            log_.err("directory iteration failed %E");
            return -1;
        }

        if (entry.is_regular_file(ec)) {
            add_file(entry.path().string());
        }
    }

    // Run update right after index if update hash is requested
    // Newly added files have lastcheck == _opts.now and
    // therefore will be ignored
    if (opts_.update()) {
        CryficUpdate cf(dbpath_);
        int res = cf.db_iterate_files();
        if (res != 0) {
            log_.err("Error updating files after indexing %d", res);
            return -1;
        }
    }

    return 0;
}

// ================ Update command
int CryficUpdate::process_row(int64_t r_id, const char *r_filename, FileStatus r_status, const char *r_hash_algo, const char *r_hash_hex) {

    const std::string& algo = (r_status == FileStatus::New || opts_.hash_override()) ? opts_.hash_algo() :  r_hash_algo;
    Hasher hs = Hasher::getHasher(algo);

    int res = hs.hash_file(r_filename);
    if (res != 0) {
        log_.err("Error hashing (%s) ... %s", _S(hs.hash_algo()), r_filename);
        db_update_hash(r_id, FileStatus::Error, NULL, NULL);
        return 1;
    }

    if (opts_.fast_update()) {
        // If the file is not changed, and has status OK or Hashed - don't update the DB
        if (r_status <= FileStatus::Hashed) {
            if (r_hash_hex == nullptr || memcmp(r_hash_hex, hs.hash_hex(), hs.hash_hex_len()) != 0) {
                log_.notice("Updating (HASHED, %s) ... %s", _S(hs.hash_algo()), r_filename);
                db_update_hash(r_id, FileStatus::Hashed, hs.hash_algo(), hs.hash_hex());
            }
        }
        else {
            log_.noise("Unchanged ... %s", r_filename);
        }
    } else {
        // Despite that the file is not changed, we are updating to reset update date.
        log_.notice("Updating (HASHED, %s) ... %s", _S(hs.hash_algo()), r_filename);
        db_update_hash(r_id, FileStatus::Hashed, hs.hash_algo(), hs.hash_hex());
    } // fast_update

    return 0;
}

int CryficUpdate::run() {
    if (opts_.purge()) {
        // Purge files marked as ERROR on the previous run
        log_.notice("Purging files with status ERROR (unreadable or missing) ... ");
        db_purge(FileStatus::Error);
    }

    return db_iterate_files();
}

// ==================== Verify command
int CryficVerify::process_row(int64_t r_id, const char *r_filename, FileStatus r_status, const char *r_hash_algo, const char *r_hash) {
    int res = -1;
    if (r_status == FileStatus::New) {
        log_.notice("Verifyng %s (%s) - NEW", r_hash_algo, r_filename);
        return 1;
    }

    // All file that are not in NEW state have to have hash algo set.
    // But if something goes wrong, we throw an exception and verification fails
    Hasher hs = Hasher::getHasher(r_hash_algo);

    // Performance, check file first
    // File doesn't exist or unreadable
    if (access(r_filename, R_OK) != 0) {
        log_.notice("Verifying %s (%s) - ERROR", r_hash_algo, r_filename);
        db_update_status(r_id, FileStatus::Error);
        return 1;
    }

    res = hs.hash_file(r_filename);
    if (res != 0) {
        // Should never reach here
        log_.notice("Verifying %s (%s) - ERROR", r_hash_algo, r_filename);
        db_update_status(r_id, FileStatus::Error);
        return 1;
    }

    if (memcmp(r_hash, hs.hash_hex(), hs.hash_hex_len()) != 0) {
        log_.notice("Verifying %s (%s) - MODIFIED", r_hash_algo, r_filename);
        db_update_status(r_id, FileStatus::Modified);
        return 1;
    }

    log_.notice("Verifying %s (%s) - OK", r_hash_algo, r_filename);

    if (!opts_.fast_update() || r_status != FileStatus::Ok) {
        // If the file is not changed, and has status OK - don't update the DB
        db_update_status(r_id, FileStatus::Ok);
    }

    return 0;
}

int CryficVerify::run() {
    int res = db_iterate_files();
    if (res == 0) {
       log_.notice("Verification completed, all OK");
    } else {
       log_.err("Verification completed with ERRORS: %d", res);
    }
    return res;
}
