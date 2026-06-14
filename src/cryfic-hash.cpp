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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include "cryfic.hpp"
#include "cryfic-logging.hpp"
#include "cryfic-hash.hpp"

/* Convert hex string to bin array, return the length of hex string */
int Hasher::bin2hex(char *hex, const uint8_t *bin, int bi_len) {
    const char * hex_digs = "0123456789ABCDEF";
    int hi = 0, bi = 0;
    while(bi < bi_len) {
        hex[hi] = hex_digs[(bin[bi] >> 4) & 0xf];
        hex[hi+1] = hex_digs[bin[bi]& 0xf];
        bi ++;
        hi += 2;
    }
    hex[hi] = 0; // ensure null termination
    return hi;
}

int Hasher::hash_for_data(const uint8_t *data, size_t length) {
    HashCTX ctx;
    uint8_t hash_bin[64];

    // EVP functions returns 0 for failure
    int err = EVP_DigestInit(ctx.ctx(), digest_);
    if (err == 0)  {
        log_.ssl_err("EVP_DigestInit() failed");
        return -1;
    }

    err = EVP_DigestUpdate(ctx.ctx(), data, length);
    if (err == 0)  {
        log_.ssl_err("EVP_DigestUpdate() failed\n");
        return -1;
    }

    err = EVP_DigestFinal(ctx.ctx(), hash_bin, (unsigned int *) &hash_len_);
    if (err == 0) {
        log_.ssl_err("EVP_DigestFinal() failed\n");
        return -1;
    }

    bin2hex(hash_hex_, hash_bin, hash_len_);
    return 0;
}

int Hasher::hash_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        log_.err("Failed to open: '%s' %E", filename);
        return -1;
    }

    // Need to know the file length
    struct stat st;
    int err = fstat(fd, &st);
    if (err < 0) {
        log_.err("Failed to stat: '%s' %E", filename);
        close(fd);
        return -1;
    }

    uint8_t *addr = (uint8_t *) mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        log_.err("Failed to mmap: '%s' %E", filename);
        close(fd);
        return -1;
    }

    int res = hash_for_data(addr, st.st_size);

    close(fd);
    munmap(addr, st.st_size);
    return res;
}