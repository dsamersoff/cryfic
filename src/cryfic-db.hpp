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

#ifndef CRYFIC_DB_HPP
#define CRYFIC_DB_HPP

#include <string>
#include <stdexcept>
#include <memory>

#include <sqlite3.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>

#include "cryfic-opts.hpp"
#include "cryfic-logging.hpp"

enum class FileStatus {
    Unknown = 0,           //! File status isn't known
    New = 1,               //! File is added to DB but not yet hashed
    Modified = 2,          //! File's hash doesn't match
    Error = 3,             //! Hash calculation failed, file doesn't exist or not readable
    Hashed = 4,            //! File is added to DB and hashed
    Ok = 5                 //! File is verified
};

// Logger that can print sqlite errors.
// Should not be used outside of DB API
class SqliteLogger: public BasicLogger {
    private:
        inline void sqlerr(int err_code, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            const char *err_msg = sqlite3_errmsg(db_);
            log_impl(LOG_ERR, err_code, err_msg, fmt, args);
            va_end(args);
        }

        sqlite3 *db_;
        SqliteLogger(sqlite3 *db): BasicLogger(), db_(db) { }

    friend class CryficDB;
};

// Shell to access database
class CryficDB {
    protected:
        std::string dbpath_;  //! Path to database with files
        sqlite3 *db_;         //! Handler for open database
        SqliteLogger log_;    //! Debug and error logger
        CryficOptions& opts_ = CryficOptions::instance();

    public:

        static sqlite3* open_db(const std::string& path) {
            sqlite3* db = nullptr;
            int res = sqlite3_open(path.c_str(), &db);
            if (res != SQLITE_OK) {
                throw std::runtime_error("Can't open database " + path);
            }
            return db;
        }

        // Create a new empty database with proper schema and indices
        static int db_create(const std::string& dbpath) {
            return db_create(dbpath.c_str());
        }

        static int db_create(const char *dbpath);

        // Open existing database
        CryficDB(const std::string& dbpath):
                   dbpath_(dbpath),
                   db_(open_db(dbpath)),
                   log_(db_) {
        }

        ~CryficDB() {
            sqlite3_close(db_);
        }

        const std::string& dbpath(){ return dbpath_; }

        virtual int process_row(int64_t id, const char *filename, FileStatus status, const char *hash_algo, const char *hash_hex);

        // Return sum of process_row returns, throw exception on error
        int db_iterate_files() {
            if (opts_.threads() > 1) {
                return db_iterate_files_mt();
            }
            return db_iterate_files_st();
        }

        // Single thread version
        int db_iterate_files_st();
        // Version that uses thread pool
        int db_iterate_files_mt();

        // Returns ID of just inserted row, throw exception on error
        int db_add_file(const std::string& filename, FileStatus status, const std::string& hash_algo, const char *hash_hex) {
            return db_add_file(filename.c_str(), status, hash_algo.empty() ? nullptr : hash_algo.c_str(), hash_hex);
        }

        int db_add_file(const char *filename, FileStatus status, const char *hash_algo, const char *hash_hex);

        void db_update_hash(int64_t id, FileStatus status, const std::string& hash_algo, const char *hash_hex) {
            db_update_hash(id, status, hash_algo.empty() ? nullptr : hash_algo.c_str(), hash_hex);
        }

        void db_update_hash(const std::string& filename, FileStatus status, const std::string& hash_algo, const char *hash_hex) {
            db_update_hash(filename.c_str(), status, hash_algo.empty() ? nullptr : hash_algo.c_str(), hash_hex);
        }

        void db_update_hash(int64_t id, FileStatus status, const char *hash_algo, const char *hash_hex);
        void db_update_hash(const char *filename, FileStatus status, const char *hash_algo, const char *hash_hex);

        void db_update_status(int64_t id, FileStatus new_status);

        void db_purge(FileStatus status);
};

struct ProcessRowTask {
    CryficDB* db;
    int64_t id;
    std::string filename;
    FileStatus status;
    std::string hash_algo;
    std::string hash_hex;
};


#endif
