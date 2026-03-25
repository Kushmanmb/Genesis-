#include "Blockchain.h"
#include "sha256.h"

#include <sstream>
#include <stdexcept>

// The zero/burn address used as the "origin" destination for token returns.
static const std::string ORIGIN_ADDRESS = "0x0000000000000000000000000000000000000000";

Blockchain::Blockchain() {
    chain.push_back(Block::createGenesis());
}

void Blockchain::requireOwner(const std::string &callerAddress) const {
    if (!isOwner(callerAddress)) {
        throw std::runtime_error("Permission denied: caller is not authorised");
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

    std::ostringstream oss;
    oss << "Return tokens to origin (" << ORIGIN_ADDRESS << "):";
    for (const auto &addr : OWNER_ADDRESSES) {
        oss << " [" << addr << "]";
    }
    addBlockInternal(oss.str());
}

void Blockchain::returnToLegacy(const std::string &callerAddress,
                                const std::string &legacyAddress) {
    requireOwner(callerAddress);

    std::ostringstream oss;
    oss << "Consolidate mytoken balances and return to legacy address ("
        << legacyAddress << "):";
    for (const auto &addr : OWNER_ADDRESSES) {
        oss << " [" << addr << "]";
    }
    addBlockInternal(oss.str());
}

void Blockchain::returnToOwner(const std::string &callerAddress) {
    requireOwner(callerAddress);

    std::ostringstream oss;
    oss << "Consolidate mytoken balances and return to owner (" << callerAddress << "):";
    for (const auto &addr : OWNER_ADDRESSES) {
        oss << " [" << addr << "]";
    }
    addBlockInternal(oss.str());
}

std::string Blockchain::chkpotpie(uint32_t fromIndex, uint32_t toIndex) const {
    if (fromIndex > toIndex) {
        throw std::out_of_range("chkpotpie: fromIndex must not exceed toIndex");
    }
    if (toIndex >= static_cast<uint32_t>(chain.size())) {
        throw std::out_of_range("chkpotpie: toIndex is out of bounds");
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
