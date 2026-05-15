//
//          Copyright (c) 2026
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//
// This file contains application wide constants

#include <cstdint>

/// @brief Number of bytes to read to determine if a file is binary or not
const std::size_t kMaxReadBinary = 64 * 1024;
