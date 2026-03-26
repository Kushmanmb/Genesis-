#pragma once

#include <cstdint>
#include <ctime>
#include <string>

class Block {
public:
    Block(uint32_t index, const std::string &data, const std::string &previousHash,
          std::time_t timestamp = std::time(nullptr));

    [[nodiscard]] uint32_t    getIndex()        const { return index; }
    [[nodiscard]] std::time_t getTimestamp()    const { return timestamp; }
    [[nodiscard]] std::string getData()         const { return data; }
    [[nodiscard]] std::string getPreviousHash() const { return previousHash; }
    [[nodiscard]] std::string getHash()         const { return hash; }

    static Block createGenesis();
    [[nodiscard]] std::string toString() const;

private:
    const uint32_t    index;
    const std::time_t timestamp;
    const std::string data;
    const std::string previousHash;
    const std::string hash;

    std::string calculateHash() const;
};
