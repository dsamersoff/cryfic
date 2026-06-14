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

#ifndef _CRYFIC_LOGGING_H_
#define _CRYFIC_LOGGING_H_

#ifdef __linux__
#define HAVE_SYSLOG
#endif

#ifdef __FreeBSD__
#define HAVE_SYSLOG
#endif

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_SYSLOG
#include <syslog.h>
#endif

#include <iostream>

#include "cryfic-opts.hpp"

#ifndef HAVE_SYSLOG
    // Align LOG_LEVELS with SYSLOG, but not all levels used
    #define LOG_EMERG   0   /* system is unusable */
    #define LOG_ALERT   1   /* action must be taken immediately */
    #define LOG_CRIT    2   /* critical conditions */
    #define LOG_ERR     3   /* error conditions */
    #define LOG_WARNING 4   /* warning conditions */
    #define LOG_NOTICE  5   /* normal but significant condition */
    #define LOG_INFO    6   /* informational */
    #define LOG_DEBUG   7   /* debug-level messages */
#endif

inline const char* _S(const std::string& s) {
    return s.c_str();
}

class BasicLogger {
    public:
        static BasicLogger& instance() {
            static BasicLogger instance;
            return instance;
        }

        // Delete copy constructor and assignment to enforce singleton
        BasicLogger(const CryficOptions&) = delete;
        BasicLogger& operator=(const CryficOptions&) = delete;

        // Delete move operations too
        BasicLogger(CryficOptions&&) = delete;
        BasicLogger& operator=(CryficOptions&&) = delete;

        inline void err(const char *fmt, ...) {
            va_list args;
            va_start(args, fmt);
            log_impl(LOG_ERR, 0, nullptr, fmt, args);
            va_end(args);
        }

        inline void warn(const char *fmt, ...) {
            va_list args;
            va_start(args, fmt);
            log_impl(LOG_WARNING, 0, nullptr, fmt, args);
            va_end(args);
        }

        inline void notice(const char *fmt, ...) {
            va_list args;
            va_start(args, fmt);
            log_impl(LOG_NOTICE, 0, nullptr, fmt, args);
            va_end(args);
        }

        inline void noise(const char *fmt, ...) {
            va_list args;
            va_start(args, fmt);
            log_impl(LOG_INFO, 0, nullptr, fmt, args);
            va_end(args);
        }

        inline void debug(const char *fmt, ...)  {
            va_list args;
            va_start(args, fmt);
            log_impl(LOG_DEBUG, 0, nullptr, fmt, args);
            va_end(args);
        }

        inline void print(const char *fmt, ...)  {
            va_list args;
            va_start(args, fmt);
            log_impl(-1, 0, nullptr, fmt, args);
            va_end(args);
        }

    protected:
        BasicLogger() : os_(std::cerr) {}

        void log_impl(int log_level, int err_code, const char *err_str, const char *format, va_list args);

    private:
        std::ostream& os_;
        const char *log_level_name(int level);

};

#endif
