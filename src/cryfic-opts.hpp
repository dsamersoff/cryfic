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

#ifndef _CRYFIC_OPTS_HPP_
#define _CRYFIC_OPTS_HPP_

#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <functional>
#include <unordered_map>
#include <variant>

class CryficOptions {
public:
    using OptionValue = std::variant<int*, std::string*>;

    static CryficOptions& instance() {
        static CryficOptions instance; // thread-safe since C++11
        return instance;
    }

    // Delete copy constructor and assignment to enforce singleton
    CryficOptions(const CryficOptions&) = delete;
    CryficOptions& operator=(const CryficOptions&) = delete;

    // Delete move operations too
    CryficOptions(CryficOptions&&) = delete;
    CryficOptions& operator=(CryficOptions&&) = delete;

    // Load options from comma-separated key=value pairs
    int loadFromEnv(const std::string& env, bool permissive = false);

    // Load options from configuration file
    int loadFromFile(const std::string& filename, bool permissive = true);

    // Update dependent options based on current values
    void updateDependentOptions();

    // Log current options state
    void log() const;

    // Getters
    int version() const { return version_; }
    int verbose() const { return verbose_; }
    int use_stderr() const { return use_stderr_; }
    int use_syslog() const { return use_syslog_; }
    int update() const { return update_; }
    int purge() const { return purge_; }
    int fast_update() const { return fast_update_; }
    int threads() const { return threads_; }
    int hash_override() const { return hash_override_; }
    const std::string& hash_algo() const { return hash_algo_; }

    // Global timestamp
    int64_t now() const { return now_; }

    // Setters
    void version(int v) { version_ = v; }
    void verbose(int v) { verbose_ = v; }
    void use_stderr(int v) { use_stderr_ = v; }
    void use_syslog(int v) { use_syslog_ = v; }
    void update(int v) { update_ = v; }
    void purge(int v) { purge_ = v; }
    void fast_update(int v) { fast_update_ = v; }
    void threads(int v) { threads_ = v; }
    void hash_override(int v) { hash_override_ = v; }
    void hash_algo(const std::string& v) { hash_algo_ = v; }

private:
    // Option values
    int version_;
    int verbose_;
    int use_stderr_;
    int use_syslog_;
    int update_;
    int purge_;
    int fast_update_;
    int threads_;
    int hash_override_;
    std::string hash_algo_;

    //Global timestamp
    int64_t now_;

    std::unordered_map<std::string, OptionValue> option_map_;

    OptionValue* findOption(const std::string& key);

    /*
     * Set the option from str, supports following formats (SPACES NOT ALLOWED):
     * enforce=[1|0]
     * hash_algo=sha512
     */
    void setOptionFromStr(const std::string& str, bool permissive);

    CryficOptions();
};

#endif