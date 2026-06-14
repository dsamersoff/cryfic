# cryfic

> High-performance multithreaded filesystem integrity checker with SQLite backend.

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-Linux-green.svg)]()
[![SQLite](https://img.shields.io/badge/database-SQLite-blue.svg)]()
[![OpenSSL](https://img.shields.io/badge/crypto-OpenSSL-orange.svg)]()

---

## Table of Contents

- [Overview](#overview)
- [Motivation](#motivation)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Workflow](#workflow)
- [Command Line Usage](#command-line-usage)
- [Command Line Options](#command-line-options)
- [Database Schema](#database-schema)
- [File Status Codes](#file-status-codes)
- [Filesystem Integrity Checkers Overview](#filesystem-integrity-checkers-overview)
- [Reporting](#reporting)
- [Examples](#examples)
- [Performance](#performance)
- [Dependencies](#dependencies)
- [Build](#build)
- [License](#license)

---

# Overview

**cryfic** is a high-performance filesystem integrity checker and file hash verification tool.

It detects file corruption, modification, and disappearance by computing cryptographic hashes and comparing them with previously stored values.

Unlike traditional integrity checkers, cryfic uses:

- lock-free multithreaded hashing
- SQLite database storage
- incremental verification workflow
- script-friendly architecture

---

# Motivation

Typical integrity checkers:

- store hashes in text files
- require complex filtering
- are difficult to automate
- are often single-threaded

Example problem:

You have:

- 15,000 important files
- 40,000 unimportant files

You want:

- fast weekly verification
- flexible filtering
- clean reporting

cryfic solves this by using a structured SQLite database that can be:

- filtered using SQL
- populated using scripts
- verified incrementally

---

# Key Features

- Lock-free multithreaded hashing
- SQLite database backend
- Incremental verification
- Script-friendly
- Fast verification cycles
- Flexible reporting via SQL
- Scales to millions of files

---

# Architecture

cryfic operates in three independent stages:

```

Filesystem → INDEX → Database → UPDATE → Database → VERIFY → Report

````

### Stage 1 — index

Scans filesystem and adds files to database.

### Stage 2 — update

Computes hashes and stores them.

### Stage 3 — verify

Recomputes hashes and compares against stored values.

---

# Workflow

Initial setup:

```bash
cryfic index -u /archive
````

Weekly verification:

```bash
cryfic verify /archive
```

Add new files:

```bash
cryfic index /archive/new_directory
```

---

# Command Line Usage

```
cryfic [index|update|verify] [options] <root_dir>
```

---

# Command Line Options

| Option          | Description                                              |
| --------------- | -------------------------------------------------------- |
| `-a <hash>`     | Hash algorithm (default: SHA256)                         |
| `-A <hash>`     | Override database hash algorithm during update           |
| `-c <file>`     | Use specific config file                                 |
| `-f <database>` | Use specific database file, default cryfic.sqlite3       |
| `-o <database>` | Same as `-f`                                             |
| `-l <level>`    | Log level (0-9), see syslog, default LOG_NOTICE          |
| `-p`            | Purge missing/unreadable files                           |
| `-s`            | Log to syslog                                            |
| `-t <threads>`  | Number of worker threads (default: auto)                 |
| `-u`            | Update hash during indexing                              |
| `-U`            | Update last check time even if file satus unchanged      |
| `-h`            | Show help                                                |

---

# Database Schema

## files table

```sql
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    filename TEXT UNIQUE NOT NULL,
    status INTEGER DEFAULT 0,
    lastcheck INTEGER DEFAULT 0,
    hash_algo TEXT,
    hash TEXT
);
```

Fields:

| Field     | Description                 |
| --------- | --------------------------- |
| id        | Unique ID                   |
| filename  | Full file path              |
| status    | Integrity status            |
| lastcheck | Last verification timestamp |
| hash_algo | Hash algorithm              |
| hash      | Stored hash                 |

---

## meta table

```sql
CREATE TABLE IF NOT EXISTS meta (
    id INTEGER PRIMARY KEY DEFAULT 1,
    version INTEGER DEFAULT 0,
    schema_version INTEGER DEFAULT 0,
    lastcheck INTEGER DEFAULT 0
);
```

Contains only one row.

---

# File Status Codes

cryfic uses the following status codes:

```cpp
enum class FileStatus {
    Unknown = 0,   // File status isn't known
    New = 1,       // File added to DB but not hashed
    Modified = 2,  // File hash doesn't match stored hash
    Error = 3,     // Hash failed, file missing, or unreadable
    Hashed = 4,    // File hashed but not verified
    Ok = 5         // File verified successfully
};
```

---

## Status Code Table

| Code | Name     | Meaning                                  |
| ---- | -------- | ---------------------------------------- |
| 0    | Unknown  | Status not yet determined                |
| 1    | New      | File indexed but not hashed              |
| 2    | Modified | File contents changed                    |
| 3    | Error    | File missing, unreadable, or hash failed |
| 4    | Hashed   | File hashed successfully                 |
| 5    | Ok       | File verified successfully               |

---

# Filesystem Integrity Checkers Overview

Integrity checkers detect:

* file corruption
* unauthorized modification
* disk errors
* bit rot
* malware tampering

They work by computing cryptographic hashes:

Examples:

* SHA256
* SHA512
* BLAKE2
* SHA3

If file content changes, hash changes.

Common tools:

* Tripwire
* AIDE
* Samhain

cryfic improves on them by providing:

* multithreading
* database backend
* script integration
* incremental workflow

---

# Reporting

Example query:

```sql
SELECT filename FROM files WHERE status != 5;
```

List modified files:

```sql
SELECT filename FROM files WHERE status = 2;
```

List missing files:

```sql
SELECT filename FROM files WHERE status = 3;
```

---

# Examples

Index files:

```bash
cryfic index /data
```

Index and hash:

```bash
cryfic index -u /data
```

Update hashes:

```bash
cryfic update /data
```

Verify files:

```bash
cryfic verify /data
```

---

# Performance

cryfic is optimized for:

* multi-core CPUs
* large archives
* SSD and HDD
* incremental verification

Typical improvements over traditional tools:

* 5-20× faster hashing
* near-linear CPU scaling
* fast incremental verification

---

# Dependencies

Required:

* OpenSSL ≥ 1.1.1
* SQLite3
* C++17 compiler

Linux recommended.

---

# Build

Example:

```bash
git clone https://github.com/yourname/cryfic.git
cd cryfic
make
```

---

# Example Automation

Weekly cron job:

```bash
cryfic verify /archive
sqlite3 cryfic.db "SELECT filename FROM files WHERE status != 5;" | mail -s "Integrity Report" user@example.com
```

---

# Design Philosophy

cryfic is designed to be:

* fast
* simple
* reliable
* script-friendly
* scalable

---

# License

Apache 2.0 License