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

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <syslog.h>

#include <cstdarg>
#include <cstdio>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>


#include "cryfic.hpp"
#include "cryfic-logging.hpp"
#include "dsForm.hpp"

#define LOG_PREFIX "cryfic"

const char *BasicLogger::log_level_name(int level) {
    static const char *LOG_LEVEL_NAMES[8] = {"EMERG", "ALERT", "CRIT", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG"};
    if (level < 0 || level > 7) {
        return "----";
    }
    return LOG_LEVEL_NAMES[level];
}

void BasicLogger::log_impl(int log_level, int err_code, const char* err_str, const char* format, va_list args) {
    if (log_level > 0 && CryficOptions::instance().verbose() < log_level) {
        return;
    }

    // Decrease interleaving of MT output.
    std::ostringstream message;
    message << LOG_PREFIX << "[" << log_level_name(log_level) << "]:";
#ifdef HAVE_SYSLOG
    int start = message.tellp();
#endif

    dsForm::vform(message, format, args);
    if (err_str != nullptr) {
        message << " - " << err_str << " (" << err_code << ")";
    }

#ifdef HAVE_SYSLOG
    if (log_level > 0 && CryficOptions::instance().use_syslog()) {
        syslog(log_level, "%s", (char *)(message.str().c_str()) + start);
    }
#endif

    message << std::endl;
    os_ << message.str();
    os_.flush();
}