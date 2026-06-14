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

#ifndef _CRYFIC_HASH_H_
#define _CRYFIC_HASH_H_

#include <string>
#include <stdexcept>

#include <stdarg.h>

#include <openssl/hmac.h>
#include <openssl/err.h>

class SSLogger: public BasicLogger {
    private:
        inline void ssl_err(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            int err_code = ERR_get_error();
            const char *err_msg = ERR_reason_error_string(err_code);
            log_impl(LOG_ERR, err_code, err_msg, fmt, args);
            va_end(args);
        }

        SSLogger(): BasicLogger() { }

    friend class Hasher;
};

class HashCTX {
    EVP_MD_CTX *ctx_;;

    public:
        HashCTX() {
            ctx_ = EVP_MD_CTX_new();
            if (ctx_ == NULL) {
                throw std::runtime_error("EVP_MD_CTX_new() failed");
            }
        }

        ~HashCTX() {
            EVP_MD_CTX_free(ctx_);
        }

        inline EVP_MD_CTX *ctx(){ return ctx_; }
};

class Hasher {
    public:
        // Return cached hasher if algo matches
        static Hasher getHasher(const char *p_hash_algo) {
            return Hasher::getHasher(std::string(p_hash_algo));
        }

        static Hasher getHasher(const std::string& p_hash_algo) {
            static Hasher cached = Hasher(CryficOptions::instance().hash_algo());
            if (cached.hash_algo_ == p_hash_algo) {
                return cached;
            }
            return Hasher(p_hash_algo);
        }

        int hash_file(const char *filename);
        int hash_file(const std::string& filename){ return hash_file(filename.c_str()); };

        inline const char *hash_hex() { return hash_hex_; }
        inline int hash_hex_len() { return hash_len_*2; }
        inline const std::string& hash_algo() { return hash_algo_; }


    private:
        std::string hash_algo_;
        int hash_len_;
        char hash_hex_[64*2+1];

        const EVP_MD *digest_;
        SSLogger log_;

        int hash_for_data(const uint8_t *data, size_t length);
        int bin2hex(char *hex, const uint8_t *bin, int bi_len);

        Hasher(const std::string& p_hash_algo):
                        hash_algo_(p_hash_algo),
                        hash_len_(0) {

            digest_ = EVP_get_digestbyname(hash_algo_.c_str());
            if (digest_ == NULL) {
                throw std::runtime_error(std::string("EVP_get_digestbyname() failed for '") + hash_algo_ + "' check -a/-A arg or ssl engines");
            }
        }
};


#endif
