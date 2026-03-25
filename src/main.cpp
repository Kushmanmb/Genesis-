#include <iostream>
#include <stdexcept>
#include "Blockchain.h"
#include "Node.h"
#include "Owners.h"

int main() {
    // Start the node
    Node node;
    node.start();
    std::cout << "Node started: " << (node.isRunning() ? "yes" : "no") << "\n";

    // Authorised owner addresses — sourced from the canonical OWNER_ADDRESSES list.
    const std::string owner1(OWNER_ADDRESSES[0]);
    const std::string owner2(OWNER_ADDRESSES[1]);

    // Owner 1 adds a block
    node.addBlock("Block 1 data", owner1);

    // Owner 2 adds a block
    node.addBlock("Block 2 data", owner2);

    // Demonstrate that a non-owner cannot add a block
    try {
        node.addBlock("Block 3 data", "0x0000000000000000000000000000000000000000");
    } catch (const std::runtime_error &e) {
        std::cerr << "Caught expected error: " << e.what() << "\n";
    }

    std::cout << "=== All Blocks ===\n";
    for (const Block &b : node.fetchAll()) {
        std::cout << b.toString();
    }

    // Stop the node
    node.stop();
    std::cout << "Node stopped: " << (!node.isRunning() ? "yes" : "no") << "\n";

    return 0;
}

