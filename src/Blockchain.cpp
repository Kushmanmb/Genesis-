#include "Blockchain.h"
#include "Logger.h"
#include "sha256.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>

// The zero/burn address used as the "origin" destination for token returns.
static const std::string ORIGIN_ADDRESS = "0x0000000000000000000000000000000000000000";

Blockchain::Blockchain() {
    chain.push_back(Block::createGenesis());
}

void Blockchain::requireOwner(const std::string &callerAddress) const {
    if (!isOwner(callerAddress)) {
        LOG_WARNING("Permission denied for caller: " + callerAddress);
        throw std::runtime_error(
            "Permission denied: caller '" + callerAddress + "' is not authorised");
    }
}

void Blockchain::addBlock(const std::string &data) {
    addBlockInternal(data);
}

void Blockchain::addBlock(const std::string &data, const std::string &callerAddress) {
    requireOwner(callerAddress);
    addBlockInternal(data);
}

void Blockchain::returnToOrigin(const std::string &callerAddress) {
    requireOwner(callerAddress);
    addBlockInternal("Return tokens to origin (" + ORIGIN_ADDRESS + "):" +
                     ownerAddressList());
}

void Blockchain::returnToLegacy(const std::string &callerAddress,
                                const std::string &legacyAddress) {
    requireOwner(callerAddress);
    addBlockInternal(
        "Consolidate mytoken balances and return to legacy address (" +
        legacyAddress + "):" + ownerAddressList());
}

void Blockchain::returnToOwner(const std::string &callerAddress) {
    requireOwner(callerAddress);
    addBlockInternal(
        "Consolidate mytoken balances and return to owner (" +
        callerAddress + "):" + ownerAddressList());
}

void Blockchain::consolidateBalances(const std::string &callerAddress) {
    requireOwner(callerAddress);
    addBlockInternal("Consolidate token balances:" + ownerAddressList());
}

std::string Blockchain::computeRangeChecksum(uint32_t fromIndex, uint32_t toIndex) const {
    if (fromIndex > toIndex) {
        throw std::out_of_range("computeRangeChecksum: fromIndex must not exceed toIndex");
    }
    if (toIndex >= static_cast<uint32_t>(chain.size())) {
        throw std::out_of_range("computeRangeChecksum: toIndex is out of bounds");
    }
    std::string combined;
    for (uint32_t i = fromIndex; i <= toIndex; ++i) {
        combined += chain[i].getHash();
    }
    return sha256(combined);
}

void Blockchain::addBlockInternal(const std::string &data) {
    if (chain.empty()) {
        throw std::runtime_error("Blockchain is in an invalid state: chain is empty");
    }
    const Block &prev = chain.back();
    chain.emplace_back(
        static_cast<uint32_t>(chain.size()),
        data,
        prev.getHash()
    );
    LOG_DEBUG("Block added index=" + std::to_string(chain.back().getIndex()) +
              " hash=" + chain.back().getHash());
}

const std::vector<Block> &Blockchain::fetchAll() const {
    return chain;
}

std::vector<Block> Blockchain::fetchAllFrom(const std::string &identifier) const {
    std::vector<Block> result;
    for (const auto &block : chain) {
        if (block.getData().find(identifier) != std::string::npos) {
            result.push_back(block);
        }
    }
    return result;
}

std::vector<std::pair<std::string, std::size_t>>
Blockchain::getTrending(std::size_t topN) const {
    if (topN == 0 || chain.empty()) {
        return {};
    }

    std::map<std::string, std::size_t> counts;
    for (const auto &block : chain) {
        counts[block.getData()]++;
    }

    std::vector<std::pair<std::string, std::size_t>> ranked(counts.begin(), counts.end());
    std::sort(ranked.begin(), ranked.end(),
              [](const std::pair<std::string, std::size_t> &a,
                 const std::pair<std::string, std::size_t> &b) {
                  return b.second < a.second;
              });

    if (ranked.size() > topN) {
        ranked.resize(topN);
    }
    return ranked;
}

bool Blockchain::validateSocialProfile() {
    LOG_DEBUG("Blockchain::validateSocialProfile profile=" + std::string(SOCIAL_PROFILE));
    addBlockInternal(std::string(SOCIAL_PROFILE));
    return !fetchAllFrom(std::string(SOCIAL_PROFILE)).empty();
}

void Blockchain::announce(const std::string &message, const std::string &callerAddress) {
    requireOwner(callerAddress);
    addBlockInternal("Announcement: " + message);
}

std::string Blockchain::ownerAddressList() {
    std::ostringstream oss;
    for (const auto &addr : OWNER_ADDRESSES) {
        oss << " [" << addr << "]";
    }
    return oss.str();
}
