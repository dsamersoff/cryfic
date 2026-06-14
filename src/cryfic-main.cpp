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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

#include "cryfic.hpp"
#include "cryfic-opts.hpp"
#include "cryfic-logging.hpp"

enum class Mode {
    Unknown,
    Index,
    Update,
    Verify
};

static void usage(const std::string& msg) {
    if (! msg.empty()) {
        std::cerr << "Command line ERROR: " << msg << std::endl;
    }

    std::cout << "Usage: cryfic [index|update|verify] [options] <root_dir>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -a hash                                   Algorithm for hashing, default: " << CRYFIC_DEFAULT_HASH_ALGO << std::endl;
    std::cout << "  -A hash                                   Algorithm for hashing, override database value on update" << std::endl;
    std::cout << "  -c <file>                                 Use the specified config file instead of the default" << std::endl;
    std::cout << "  -f database                               Use the specified database instead of the default " << CRYFIC_DEFAULT_DBNAME << std::endl;
    std::cout << "  -l <number>                               Log level 0 - 9, default " << CRYFIC_DEFAULT_LOG_LEVEL << std::endl;
    std::cout << "  -o database                               Use the specified database instead of the default (same as -f) " << std::endl;
    std::cout << "  -p                                        Purge non-existing and not readable files from a database (default NO)" << std::endl;
    std::cout << "  -s                                        Log to syslog (default NO)" << std::endl;
    std::cout << "  -t                                        Number of threads to update and verify (default AUTO)" << std::endl;
    std::cout << "  -u                                        Update hash during indexing (default NO)" << std::endl;
    std::cout << "  -U                                        Update file check date ever if file status is not changed (default NO)" << std::endl;
    std::cout << "  -h                                        Show this help message." << std::endl;
    exit(7);
}

int main(int argc, char *argv[]) {

    std::cerr << "CRYFIC file integrity checker v. " << CRYFIC_VERSION << " " << GIT_VERSION << std::endl;

    Mode o_mode = Mode::Unknown;
    int o_log_level = -1;
    int o_syslog = -1;
    int o_update = -1;
    int o_fast_update = -1;
    int o_hash_override = -1;
    int o_purge = -1;
    int o_threads = -1;

    std::string o_hash_algo;
    std::string o_config_file;
    std::string o_database;

    // Set mode
    if (argc > 1) {
        if (strcmp(argv[1], "index") == 0) {
            o_mode = Mode::Index;
        }
        else if (strcmp(argv[1], "update") == 0) {
            o_mode = Mode::Update;
        }
        else if (strcmp(argv[1], "verify") == 0) {
            o_mode = Mode::Verify;
        }
    }

    if (o_mode == Mode::Unknown) {
        std::string msg("Invalid operation mode should be index, update, verify");
        if (argc > 1) {
            msg += std::string("provided: ") + std::string(argv[1]);
        }

        usage(msg);
    }

    int c;
    while ((c = getopt(argc - 1, argv + 1, "a:A:c:f:l:o:pst:uUh")) != -1) {
        switch (c) {
            case 'a':
                o_hash_algo = std::string(optarg);
                break;
            case 'A':
                o_hash_algo = std::string(optarg);
                o_hash_override = 1;
                break;
            case 'c':
                o_config_file = std::string(optarg); // Alter location of config file
                break;
            case 'o': // -o is same as -f for compatibility with old scripts
            case 'f':
                o_database = std::string(optarg); // Alter location of database
                break;
            case 'l':
                o_log_level = atoi(optarg); // Explicitly set log level
                break;
            case 'p':
                o_purge = 1;
                break;
            case 's':
                o_syslog = 1;
                break;
            case 't':
                o_threads = atoi(optarg); // Explicitly set number of threads
                break;
            case 'u':
                o_update = 1;
                break;
            case 'U':
                o_fast_update = 0;
                break;
            case 'h':
                usage(NULL);
                break;
            default:
                usage( std::string("Bad option -") + std::string(1, c) );
        }
    }

    // Command line parameters validation
    if (o_mode == Mode::Index) {
        if (optind == argc || argv[optind + 1] == NULL) {
            usage("Directory is required");
        }
    }

    if (! o_config_file.empty() && access(o_config_file.c_str(), R_OK) != 0) {
        std::cerr << "ERROR: User config file doesn't exists or unreadable" << std::endl;
        return 7;
    }

    CryficOptions& opts = CryficOptions::instance();

    // The init function sets global _opts variable to final values
    // after processing of all possible options sources
    // and create the database if required
    std::string dbname = (! o_database.empty()) ? o_database : std::string(CRYFIC_DEFAULT_DBNAME);
    cryfic_init(o_config_file, dbname);

    // Command line override all other sources
    if (o_log_level >= 0) opts.verbose(o_log_level);
    if (o_update >= 0) opts.update(o_update);
    if (o_update >= 0) opts.fast_update(o_fast_update);
    if (o_syslog >= 0) opts.use_syslog(o_syslog);
    if (o_hash_override >= 0) opts.hash_override(o_hash_override);
    if (o_purge >= 0) opts.purge(o_purge);
    if (o_threads >= 0) opts.threads(o_threads); // 1 or more threads to run
    if (! o_hash_algo.empty()) opts.hash_algo(o_hash_algo);

    opts.log();

    BasicLogger& log = BasicLogger::instance();

    int res;
    if (o_mode == Mode::Index) {
        try {
            std::string root_dir(argv[optind + 1]);
            log.noise("Starting index mode for %s (%s)", _S(root_dir), _S(dbname));
            CryficIndex cf(dbname);
            cf.run(root_dir);
            exit(0);
        } catch (const std::exception& e) {
            std::cerr << "Error: INDEX mode failed " << e.what() << std::endl;
            exit(1);
        }
    }

    if (o_mode == Mode::Update) {
        try {
            log.noise("Starting update mode for %s", _S(dbname));
            CryficUpdate cf(dbname);
            cf.run();
            exit(0);
        } catch (const std::exception& e) {
            std::cerr << "Error: INDEX mode failed " << e.what() << std::endl;
            exit(1);
        }
    }

    if (o_mode == Mode::Verify) {
        try {
            log.noise("Starting verify mode for %s", _S(dbname));
            CryficVerify cf(dbname);
            res = cf.run();
            exit(res);
        } catch (const std::exception& e) {
            std::cerr << "Error: INDEX mode failed " << e.what() << std::endl;
            exit(1);
        }
    }

    usage("Unhandled combination of options");
    return 1;
}
