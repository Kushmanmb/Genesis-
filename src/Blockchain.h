#pragma once

#include "Block.h"
#include <vector>

class Blockchain {
public:
    Blockchain();

    void addBlock(const std::string &data);
    const std::vector<Block> &fetchAll() const;

private:
    std::vector<Block> chain;
};
