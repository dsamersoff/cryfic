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

#include <string>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cryfic-logging.hpp"
#include "cryfic-db.hpp"
#include "cryfic-threads.hpp"
#include "cryfic.hpp"

/* SQL statements */
static const char *SQL_CREATE_TABLE_FILES =
    "CREATE TABLE IF NOT EXISTS files ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    filename TEXT UNIQUE NOT NULL,"
    "    status INTEGER DEFAULT 0,"
    "    lastcheck INTEGER DEFAULT 0,"
    "    hash_algo TEXT,"
    "    hash TEXT"
    ");";

static const char *SQL_CREATE_INDEX =
    "CREATE INDEX IF NOT EXISTS idx_filename ON files(filename);";

static const char *SQL_INSERT_FILE =
    "INSERT INTO files (filename, status, lastcheck, hash_algo, hash) "
    "VALUES (?, ?, ?, ?, ?)";

static const char *SQL_SELECT_ALL =
    "SELECT id, filename, status, lastcheck, "
    "hash_algo, hash FROM files WHERE lastcheck < ? ORDER BY filename;";

static const char *SQL_UPDATE_HASH =
    "UPDATE files SET hash_algo = ?, hash = ?, status = ?, lastcheck = ? WHERE id = ?;";

static const char *SQL_UPDATE_HASH_FN =
    "UPDATE files SET hash_algo = ?, hash = ?, status = ?, lastcheck = ? WHERE filename = ?;";

static const char *SQL_UPDATE_STATUS =
    "UPDATE files SET status = ?, lastcheck = ? WHERE id = ?;";

static const char *SQL_PURGE =
    "DELETE FROM files WHERE (status = ?) AND (lastcheck < ?);";

// This table should contain the only record
static const char *SQL_CREATE_TABLE_META =
    "CREATE TABLE IF NOT EXISTS meta ("
    "    id INTEGER PRIMARY KEY DEFAULT 1,"
    "    version INTEGER DEFAULT 0,"
    "    schema_version INTEGER DEFAULT 0,"
    "    lastcheck INTEGER DEFAULT 0,"
    ");";

static const char *SQL_INSERT_META =
    "INSERT OR IGNORE INTO meta (version, schema_version, lastcheck) "
    "VALUES (?, ?, ?)";

int CryficDB::db_create(const char *dbpath) {
    char *errmsg = NULL;
    sqlite3 *db = open_db(dbpath);

    int res = sqlite3_open(dbpath, &db);
    if (res != SQLITE_OK) {
        return -1;
    }

    /* Enable foreign keys and WAL mode for better performance */
    // sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    // sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);

    // Create schema
    res = sqlite3_exec(db, SQL_CREATE_TABLE_FILES, NULL, NULL, &errmsg);
    if (res != SQLITE_OK) {
        // log_->err(res, "Can't create schema '%s'", db_path);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return -1;
    }

    res = sqlite3_exec(db, SQL_CREATE_INDEX, NULL, NULL, &errmsg);
    if (res != SQLITE_OK) {
        // log_sqlerr(&ctx, "Can't create index '%s'", db_path);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return -1;
    }

    res = sqlite3_exec(db, SQL_CREATE_TABLE_META, NULL, NULL, &errmsg);
    if (res != SQLITE_OK) {
        // log_->err(res, "Can't create schema '%s'", db_path);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_stmt *stmt;
    res = sqlite3_prepare_v2(db, SQL_INSERT_META, -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        // log_sqlerr(ctx, "Can't prepare INSERT_FILE");
        throw std::runtime_error("Can't add file - prepare failed " + std::to_string(res));
    }

    sqlite3_bind_int(stmt, 1, (int) CRYFIC_VERSION);
    sqlite3_bind_int(stmt, 2, (int) CRYFIC_SCHEMA_VERSION);
    sqlite3_bind_int64(stmt, 3, CryficOptions::instance().now());

    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE) {
        throw std::runtime_error("Can't add meta - query failed " + std::to_string(res));
    }

    sqlite3_close(db);
    return 0;
}

int CryficDB::db_add_file(const char *filename, FileStatus status, const char *hash_algo, const char *hash_hex) {
    sqlite3_stmt *stmt;
    int res = sqlite3_prepare_v2(db_, SQL_INSERT_FILE, -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        // log_sqlerr(ctx, "Can't prepare INSERT_FILE");
        throw std::runtime_error("Can't add file - prepare failed " + std::to_string(res));
    }

    sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int) status);
    sqlite3_bind_int64(stmt, 3, opts_.now());
    sqlite3_bind_text(stmt, 4, hash_algo, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, hash_hex, -1, SQLITE_STATIC);

    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE && res != SQLITE_CONSTRAINT) {
        // log_sqlerr(ctx, "Can't execute INSERT FILE");
        throw std::runtime_error("Can't add file - query failed " + std::to_string(res));
    }

    // SQLite has no reliable way to determine whether a record was actually inserted
    // when using an `INSERT OR IGNORE` query. If a constraint violation occurs,
    // `last_insert_rowid` is not reset and still points to the last successfully
    // inserted row. This behavior is not suitable here, so the error is propagated
    // to the application layer.

    // return sqlite3_last_insert_rowid(db_);
    return (res == SQLITE_DONE)? 0 : -1;
}

int CryficDB::process_row(int64_t id, const char *filename, FileStatus status, const char *hash_algo, const char *hash_hex) {
    throw std::runtime_error("Call to non-implemented method process_row");
}

int CryficDB::db_iterate_files_st() {
    sqlite3_stmt *stmt;
    int res = sqlite3_prepare_v2(db_, SQL_SELECT_ALL, -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        // log_sqlerr(ctx, "SELECT_ALL failed");
        throw std::runtime_error("Can't iterate files - prepare failed " + std::to_string(res));
    }

    sqlite3_bind_int64(stmt, 1, opts_.now()); // lastcheck
    int cb_result = 0;

    while ((res = sqlite3_step(stmt)) == SQLITE_ROW) {
        int64_t r_id = sqlite3_column_int64(stmt, 0);
        const char *r_filename = (const char *) sqlite3_column_text(stmt, 1);
        FileStatus r_status  = (FileStatus) sqlite3_column_int(stmt, 2);
        const char *r_hash_algo = (const char *) sqlite3_column_text(stmt, 4);
        const char *r_hash_hex = (const char *) sqlite3_column_text(stmt, 5);

        cb_result += process_row(r_id, r_filename, r_status, r_hash_algo, r_hash_hex);
    }

    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE) {
        throw std::runtime_error("Can't iterate files - query failed " + std::to_string(res));
    }

    return cb_result;
}

int CryficDB::db_iterate_files_mt() {
    sqlite3_stmt *stmt;

    int res = sqlite3_prepare_v2(db_, SQL_SELECT_ALL, -1, &stmt, NULL);

    if (res != SQLITE_OK) {
        throw std::runtime_error("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, opts_.now());

    static MPMCWorkerPool pool(opts_.threads(), 4096);

    while ((res = sqlite3_step(stmt)) == SQLITE_ROW) {
        int64_t r_id = sqlite3_column_int64(stmt, 0);
        const char *r_filename = (const char *) sqlite3_column_text(stmt, 1);
        FileStatus r_status = (FileStatus) sqlite3_column_int(stmt, 2);
        const char *r_hash_algo = (const char *) sqlite3_column_text(stmt, 4);
        const char *r_hash_hex = (const char *) sqlite3_column_text(stmt, 5);

        pool.submit(ProcessRowTask{
            this,
            r_id,
            std::string(r_filename ? r_filename : ""),
            r_status,
            std::string(r_hash_algo ? r_hash_algo : ""),
            std::string(r_hash_hex ? r_hash_hex : "")
        });
    }

    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE) {
        throw std::runtime_error("query failed");
    }

    pool.wait();
    return pool.get_result();
}

void CryficDB::db_update_hash(int64_t id, FileStatus status, const char *hash_algo, const char *hash_hex) {
    sqlite3_stmt *stmt;

    int res = sqlite3_prepare_v2(db_, SQL_UPDATE_HASH, -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        // log_sqlerr(ctx, "UPDATE_HASH prepare failed");
        throw std::runtime_error("Can't update hash - prepare failed " + std::to_string(res));
    }

    // Bind parameters: hash, status, lastcheck, id
    sqlite3_bind_text(stmt, 1, hash_algo, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, (const char *) hash_hex, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, (int) status);
    sqlite3_bind_int64(stmt, 4, opts_.now());
    sqlite3_bind_int64(stmt, 5, id);

    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE) {
        throw std::runtime_error("Can't update hash - query failed " + std::to_string(res));
    }
}

void CryficDB::db_update_hash(const char *filename, FileStatus status, const char *hash_algo, const char *hash_hex) {
    sqlite3_stmt *stmt;

    int res = sqlite3_prepare_v2(db_, SQL_UPDATE_HASH_FN, -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        // log_sqlerr(ctx, "UPDATE_HASH prepare failed");
        throw std::runtime_error("Can't update hash - prepare failed " + std::to_string(res));
    }

    // Bind parameters: hash, status, lastcheck, id
    sqlite3_bind_text(stmt, 1, hash_algo, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, (const char *) hash_hex, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, (int) status);
    sqlite3_bind_int64(stmt, 4, opts_.now());
    sqlite3_bind_text(stmt, 5, filename, -1, SQLITE_STATIC);

    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE) {
        throw std::runtime_error("Can't update hash - query failed " + std::to_string(res));
    }
}


void CryficDB::db_update_status(int64_t id, FileStatus new_status) {
    sqlite3_stmt *stmt;

    int res = sqlite3_prepare_v2(db_, SQL_UPDATE_STATUS, -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        // log_sqlerr(ctx, "UPDATE_STATUS prepare failed");
        throw std::runtime_error("Can't update status - prepare failed " + std::to_string(res));
    }

    sqlite3_bind_int(stmt, 1, (int) new_status);
    sqlite3_bind_int64(stmt, 2, opts_.now());
    sqlite3_bind_int64(stmt, 3, id);

    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE) {
        // log_sqlerr(ctx, "UPDATE_STATUS failed");
        throw std::runtime_error("Can't update status - query failed " + std::to_string(res));
    }
}

void CryficDB::db_purge(FileStatus status) {
    sqlite3_stmt *stmt;

    int res = sqlite3_prepare_v2(db_, SQL_PURGE, -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        // log_sqlerr(ctx, "PURGE prepare failed");
        throw std::runtime_error("Can't purge - prepare failed " + std::to_string(res));
    }

    sqlite3_bind_int(stmt, 1, (int) status);
    sqlite3_bind_int64(stmt, 2, opts_.now());

    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (res != SQLITE_DONE) {
        // log_sqlerr(ctx, "PURGE failed");
        throw std::runtime_error("Can't purge - query failed " + std::to_string(res));
    }
}
