#include "Blockchain.h"

#include <stdexcept>

Blockchain::Blockchain() {
    chain.push_back(Block::createGenesis());
}

void Blockchain::addBlock(const std::string &data) {
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
