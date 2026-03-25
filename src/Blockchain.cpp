#include "Blockchain.h"

#include <sstream>
#include <stdexcept>

// The zero/burn address used as the "origin" destination for token returns.
static const std::string ORIGIN_ADDRESS = "0x0000000000000000000000000000000000000000";

Blockchain::Blockchain() {
    chain.push_back(Block::createGenesis());
}

void Blockchain::addBlock(const std::string &data) {
    addBlockInternal(data);
}

void Blockchain::addBlock(const std::string &data, const std::string &callerAddress) {
    if (!isOwner(callerAddress)) {
        throw std::runtime_error("Permission denied: caller is not authorised");
    }
    addBlockInternal(data);
}

void Blockchain::returnToOrigin(const std::string &callerAddress) {
    if (!isOwner(callerAddress)) {
        throw std::runtime_error("Permission denied: caller is not authorised");
    }

    std::ostringstream oss;
    oss << "Return tokens to origin (" << ORIGIN_ADDRESS << "):";
    for (const auto &addr : OWNER_ADDRESSES) {
        oss << " [" << addr << "]";
    }
    addBlockInternal(oss.str());
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
