#include <iostream>
#include <stdexcept>
#include "Blockchain.h"
#include "Owners.h"

int main() {
    Blockchain bc;

    // Authorised owner addresses — sourced from the canonical OWNER_ADDRESSES list.
    const std::string owner1(OWNER_ADDRESSES[0]);
    const std::string owner2(OWNER_ADDRESSES[1]);

    // Owner 1 adds a block
    bc.addBlock("Block 1 data", owner1);

    // Owner 2 adds a block
    bc.addBlock("Block 2 data", owner2);

    // Demonstrate that a non-owner cannot add a block
    try {
        bc.addBlock("Block 3 data", "0x0000000000000000000000000000000000000000");
    } catch (const std::runtime_error &e) {
        std::cerr << "Caught expected error: " << e.what() << "\n";
    }

    // Return all kushmanmb tokens back to origin
    bc.returnToOrigin(owner1);

    std::cout << "=== All Blocks ===\n";
    for (const Block &b : bc.fetchAll()) {
        std::cout << b.toString();
    }

    // chkpotpie: SHA-256 path over all blocks in the chain
    const auto &chain = bc.fetchAll();
    if (!chain.empty()) {
        std::cout << "chkpotpie(0, " << chain.size() - 1 << "): "
                  << bc.chkpotpie(0, static_cast<uint32_t>(chain.size() - 1)) << "\n";
    }

    return 0;
}
