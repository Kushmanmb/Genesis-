#include <iostream>
#include "Blockchain.h"

int main() {
    Blockchain bc;
    bc.addBlock("Block 1 data");
    bc.addBlock("Block 2 data");

    std::cout << "=== All Blocks ===\n";
    for (const Block &b : bc.fetchAll()) {
        std::cout << b.toString();
    }
    return 0;
}
