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

    // Consolidate all mytoken balances held by owner addresses and transfer them
    // to the specified legacy address. Records the transfer as a block in the chain;
    // callerAddress must be an owner.
    void returnToLegacy(const std::string &callerAddress, const std::string &legacyAddress);

    // Consolidate all mytoken balances held by owner addresses and transfer them
    // back to the calling owner's own address. Records the transfer as a block in
    // the chain; callerAddress must be an owner.
    void returnToOwner(const std::string &callerAddress);

    // Compute a SHA-256 checksum over the hashes of blocks [fromIndex, toIndex].
    // Throws std::out_of_range if the range is invalid.
    std::string chkpotpie(uint32_t fromIndex, uint32_t toIndex) const;

    const std::vector<Block> &fetchAll() const;

    // Return all blocks whose data field contains the given identifier string.
    std::vector<Block> fetchAllFrom(const std::string &identifier) const;

private:
    // Throws if callerAddress is not an authorised owner.
    void requireOwner(const std::string &callerAddress) const;

    // Internal helper — no ownership check; only callable from within the class.
    void addBlockInternal(const std::string &data);

    std::vector<Block> chain;
};
