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
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>
#include <iostream>
#include <cctype>
#include <functional>
#include <unordered_map>
#include <variant>

#include "cryfic.hpp"
#include "cryfic-opts.hpp"

static int64_t time_ms() {
    struct timespec t;
#ifdef __FreeBSD__
    clock_gettime(CLOCK_MONOTONIC, &t);
#else
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
#endif
    return t.tv_sec *  1000000 + t.tv_nsec / 1000;
}

CryficOptions::CryficOptions():
    // Hardcoded defaults
    version_(CRYFIC_VERSION),
    verbose_(5 /* LOG_NOTICE */),                  // Don't log NOISE and DEBUG
    use_stderr_(1),                                // Log to STDERR
    use_syslog_(0),                                // Don't use syslog
    update_(0),                                    // Don't update hashes while indexing
    purge_(0),                                     // Keep files with ERROR status (missing or unreadable) in the db
    fast_update_(1),                               // Don't reset update date if file is unchanged
    threads_(std::thread::hardware_concurrency()), // Use c++ detection mechanism
    hash_override_(0),                             // Respect DB hash algo field
    hash_algo_(CRYFIC_DEFAULT_HASH_ALGO)           // Set default hash algo
{

    option_map_ = {
        {"version",       &version_},
        {"verbose",       &verbose_},
        {"stderr",        &use_stderr_},
        {"syslog",        &use_syslog_},
        {"update",        &update_},
        {"purge",         &purge_},
        {"fast_update",   &fast_update_},
        {"threads",       &threads_},
        {"hash_override", &hash_override_},
        {"hash_algo",     &hash_algo_}
    };

    // Set global timestamp
    now_ = time_ms();
}

// Load options from comma-separated key=value pairs
int CryficOptions::loadFromEnv(const std::string& env, bool permissive) {
    std::istringstream stream(env);
    std::string opt;

    while (std::getline(stream, opt, ',')) {
        setOptionFromStr(opt, permissive);
    }

    return 0;
}

// Load options from configuration file
int CryficOptions::loadFromFile(const std::string& filename, bool permissive) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return -1;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') {
            // Strip trailing whitespace
            size_t end = line.find_first_of(" \n\r");
            if (end != std::string::npos) {
                line = line.substr(0, end);
            }

            setOptionFromStr(line, permissive);
        }
    }

    return 0;
}

    // Update dependent options based on current values
void CryficOptions::updateDependentOptions() {
    if (verbose_ >= 5 /* LOG_NOTICE */) {
        use_stderr(1); // noise implies stderr
    }
}

// Log current options state
void CryficOptions::log() const {
    BasicLogger::instance().warn(
        "Options (v.%x): VERBOSE=%d, USE_STDERR=%d, USE_SYSLOG=%d, UPDATE=%d, "\
        "HASH_ALGO=%s, PURGE=%d, HASH_OVERRIDE=%d, FAST_UPDATE=%d, THREADS=%d",
                     version(), verbose(), use_stderr(), use_syslog(),
                     update(), _S(hash_algo()), purge(), hash_override(),
                     fast_update(), threads());
}

CryficOptions::OptionValue* CryficOptions::findOption(const std::string& key) {
    for (auto& [name, value] : option_map_) {
        if (strcasecmp(_S(name), _S(key)) == 0) {
            return &value;
        }
    }
    return nullptr;
}

 /*
  * Set the option from str, supports following formats (SPACES NOT ALLOWED):
  * enforce=[1|0]
  * hash_algo=sha512
  */
void CryficOptions::setOptionFromStr(const std::string& str, bool permissive) {
    size_t eq_pos = str.find('=');
    if (eq_pos == std::string::npos) {
        // Malformed option silently ignored
        return;
    }

    std::string key = str.substr(0, eq_pos);
    std::string value_str = str.substr(eq_pos + 1);

    #if 0
        if (!permissive) {
            // TODO: Some options can be changed only if they come from the permissive source
            return;
        }
    #endif

    OptionValue* option = findOption(key);
    if (option == nullptr) {
        return;
    }

    if (auto int_ptr = std::get_if<int*>(option)) {
        **int_ptr = std::atoi(_S(value_str));
    }
    else if (auto str_ptr = std::get_if<std::string*>(option)) {
        **str_ptr = value_str;
    }
}
