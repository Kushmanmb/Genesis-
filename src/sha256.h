#pragma once

#include <string>

// Computes the SHA-256 hash of the given input.
// Returns a 64-character lowercase hexadecimal string.
std::string sha256(const std::string &input);
