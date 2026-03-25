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

    // Blockchain instance for demonstrating block operations.
    Blockchain bc;

    // Add blocks anonymously — no contributors are configured.
    bc.addBlock("Block 1 data");
    bc.addBlock("Block 2 data");

    // Demonstrate that all owner-gated operations are rejected (no contributors).
    try {
        bc.addBlock("Block 3 data", "0x0000000000000000000000000000000000000000");
    } catch (const std::runtime_error &e) {
        std::cerr << "Caught expected error (addBlock): " << e.what() << "\n";
    }

    try {
        bc.returnToOrigin("0x0000000000000000000000000000000000000000");
    } catch (const std::runtime_error &e) {
        std::cerr << "Caught expected error (returnToOrigin): " << e.what() << "\n";
    }

    try {
        bc.returnToLegacy("0x0000000000000000000000000000000000000000",
                          "0xB29380d2FC97F857E1D7De0cB3F1E2b8dc5caf23");
    } catch (const std::runtime_error &e) {
        std::cerr << "Caught expected error (returnToLegacy): " << e.what() << "\n";
    }

    try {
        bc.returnToOwner("0x0000000000000000000000000000000000000000");
    } catch (const std::runtime_error &e) {
        std::cerr << "Caught expected error (returnToOwner): " << e.what() << "\n";
    }

    // Demonstrate fetchAllFrom: add blocks referencing the owner's social profile,
    // Facebook profile, and Coinbase ID, then fetch all blocks that contain each identifier.
    const std::string socialProfile(SOCIAL_PROFILE);
    const std::string facebookProfile(FACEBOOK_PROFILE);
    const std::string coinbaseId(COINBASE_ID);
    bc.addBlock("Data linked to " + socialProfile);
    bc.addBlock("Data linked to " + facebookProfile);
    bc.addBlock("Data linked to " + coinbaseId);

    std::cout << "\n=== Blocks referencing " << socialProfile << " ===\n";
    for (const Block &b : bc.fetchAllFrom(socialProfile)) {
        std::cout << b.toString();
    }

    std::cout << "\n=== Blocks referencing " << facebookProfile << " ===\n";
    for (const Block &b : bc.fetchAllFrom(facebookProfile)) {
        std::cout << b.toString();
    }

    std::cout << "\n=== Blocks referencing " << coinbaseId << " ===\n";
    for (const Block &b : bc.fetchAllFrom(coinbaseId)) {
        std::cout << b.toString();
    }

    std::cout << "=== All Blocks ===\n";
    for (const Block &b : bc.fetchAll()) {
        std::cout << b.toString();
    }

    // Demonstrate getTrending: add several blocks referencing the social profile
    // multiple times so that it rises to the top of the trending list.
    bc.addBlock("Data linked to " + socialProfile);
    bc.addBlock("Data linked to " + socialProfile);

    std::cout << "\n=== Top 3 Trending Block Data ===\n";
    for (const auto &entry : bc.getTrending(3)) {
        std::cout << "  [" << entry.second << "x] " << entry.first << "\n";
    }

    // Stop the node
    node.stop();
    std::cout << "Node stopped: " << (!node.isRunning() ? "yes" : "no") << "\n";

    return 0;
}

