#pragma once

#include "Block.h"
#include "Owners.h"
#include <string>
#include <vector>

class Blockchain {
public:
    Blockchain();

    // Append a block without an ownership check.
    void addBlock(const std::string &data);

    // Append a block; the caller must supply an owner address for authorisation.
    void addBlock(const std::string &data, const std::string &callerAddress);

    // Transfer all tokens held by owner addresses back to the origin address.
    // Records the transfer as a block in the chain; callerAddress must be an owner.
    void returnToOrigin(const std::string &callerAddress);

    const std::vector<Block> &fetchAll() const;

private:
    // Internal helper — no ownership check; only callable from within the class.
    void addBlockInternal(const std::string &data);

    std::vector<Block> chain;
};
