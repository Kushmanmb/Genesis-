#include "Block.h"
#include "sha256.h"

#include <sstream>

// Block implementation ---------------------------------------------------------

static const std::string ZERO_HASH(64, '0');

Block::Block(uint32_t idx, const std::string &blockData, const std::string &prevHash,
             std::time_t ts)
    : index(idx), timestamp(ts), data(blockData), previousHash(prevHash),
      hash(calculateHash()) {
}

std::string Block::calculateHash() const {
    std::ostringstream oss;
    oss << index << timestamp << data << previousHash;
    return sha256(oss.str());
}

Block Block::createGenesis() {
    return Block(0, "Genesis Block", ZERO_HASH);
}

std::string Block::toString() const {
    std::ostringstream oss;
    oss << "Block #"        << index        << "\n"
        << "  Timestamp  : " << timestamp    << "\n"
        << "  Data       : " << data         << "\n"
        << "  Prev Hash  : " << previousHash << "\n"
        << "  Hash       : " << hash         << "\n";
    return oss.str();
}
