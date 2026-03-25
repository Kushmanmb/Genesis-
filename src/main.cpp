#include <iostream>
#include <stdexcept>
#include "Blockchain.h"
#include "Owners.h"

int main() {
    Blockchain bc;

    // Authorised owner addresses
    const std::string owner1 = "0x6fb9e80dDd0f5DC99D7cB38b07e8b298A57bF253";
    const std::string owner2 = "0x0540e1dA908D032D2F74D50C06397cB5f2cbfDdB";

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

    std::cout << "=== All Blocks ===\n";
    for (const Block &b : bc.fetchAll()) {
        std::cout << b.toString();
    }
    return 0;
}
