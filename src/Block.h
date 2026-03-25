#pragma once

#include <cstdint>
#include <ctime>
#include <string>

class Block {
public:
    Block(uint32_t index, const std::string &data, const std::string &previousHash,
          std::time_t timestamp = std::time(nullptr));

    uint32_t    getIndex()        const { return index; }
    std::time_t getTimestamp()    const { return timestamp; }
    std::string getData()         const { return data; }
    std::string getPreviousHash() const { return previousHash; }
    std::string getHash()         const { return hash; }

    static Block createGenesis();
    std::string toString() const;

private:
    const uint32_t    index;
    const std::time_t timestamp;
    const std::string data;
    const std::string previousHash;
    const std::string hash;

    std::string calculateHash() const;
};
