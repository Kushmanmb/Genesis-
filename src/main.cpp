#include <iostream>
#include "Block.h"

int main() {
    Block genesis = Block::createGenesis();
    std::cout << "=== Genesis Block ===\n" << genesis.toString();
    return 0;
}
