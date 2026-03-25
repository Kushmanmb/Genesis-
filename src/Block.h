#pragma once

#include <cstdint>
#include <ctime>
#include <string>

class Block {
public:
    uint32_t index;
    std::time_t timestamp;
    std::string data;
    std::string previousHash;
    std::string hash;

    Block(uint32_t index, const std::string &data, const std::string &previousHash);

    static Block createGenesis();
    std::string toString() const;

private:
    std::string calculateHash() const;
};
