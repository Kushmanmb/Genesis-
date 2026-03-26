#pragma once

#include "Block.h"
#include "Owners.h"
#include <string>
#include <utility>
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

    // Consolidate all mytoken balances held by owner addresses into a single
    // chain record without routing to a specific destination address.
    // Records the consolidation as a block in the chain; callerAddress must be an owner.
    void consolidateBalances(const std::string &callerAddress);

    // Compute a SHA-256 checksum over the hashes of blocks [fromIndex, toIndex].
    // Throws std::out_of_range if the range is invalid.
    std::string chkpotpie(uint32_t fromIndex, uint32_t toIndex) const;

    const std::vector<Block> &fetchAll() const;

    // Return all blocks whose data field contains the given identifier string.
    std::vector<Block> fetchAllFrom(const std::string &identifier) const;

    // Return the top-N most frequently appearing data strings across all blocks,
    // sorted by frequency descending.  Each entry is a (data, count) pair.
    // If topN is 0 or the chain is empty, returns an empty vector.
    std::vector<std::pair<std::string, std::size_t>> getTrending(std::size_t topN = 5) const;

    // Broadcast an announcement by recording "Announcement: <message>" as a block
    // on the chain; callerAddress must be an authorised owner.
    void announce(const std::string &message, const std::string &callerAddress);

    // Record the owner's social profile (SOCIAL_PROFILE) as a block on the chain
    // and return true if the profile is confirmed in the chain afterwards.
    bool validateSocialProfile();

private:
    // Throws if callerAddress is not an authorised owner.
    void requireOwner(const std::string &callerAddress) const;

    // Internal helper — no ownership check; only callable from within the class.
    void addBlockInternal(const std::string &data);

    std::vector<Block> chain;
};
