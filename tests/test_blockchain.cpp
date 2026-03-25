#include <gtest/gtest.h>
#include "Block.h"
#include "Blockchain.h"

// ---- Block tests -------------------------------------------------------

TEST(BlockTest, ConstructorStoresFields) {
    std::time_t ts = 1000000;
    Block b(1, "hello", "prev", ts);

    EXPECT_EQ(b.getIndex(), 1u);
    EXPECT_EQ(b.getData(), "hello");
    EXPECT_EQ(b.getPreviousHash(), "prev");
    EXPECT_EQ(b.getTimestamp(), ts);
}

TEST(BlockTest, HashIsDeterministic) {
    std::time_t ts = 1000000;
    Block b1(1, "hello", "prev", ts);
    Block b2(1, "hello", "prev", ts);

    EXPECT_EQ(b1.getHash(), b2.getHash());
}

TEST(BlockTest, HashChangesWithData) {
    std::time_t ts = 1000000;
    Block b1(1, "hello", "prev", ts);
    Block b2(1, "world", "prev", ts);

    EXPECT_NE(b1.getHash(), b2.getHash());
}

TEST(BlockTest, HashIsHex64Chars) {
    std::time_t ts = 1000000;
    Block b(0, "data", "0000", ts);
    const std::string &h = b.getHash();

    ASSERT_EQ(h.size(), 64u);
    for (char c : h) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character: " << c;
    }
}

TEST(BlockTest, GenesisBlockHasIndexZero) {
    Block g = Block::createGenesis();
    EXPECT_EQ(g.getIndex(), 0u);
    EXPECT_EQ(g.getData(), "Genesis Block");
    EXPECT_EQ(g.getPreviousHash(), std::string(64, '0'));
}

TEST(BlockTest, ToStringContainsFields) {
    std::time_t ts = 1000000;
    Block b(2, "test data", "abcd", ts);
    std::string s = b.toString();

    EXPECT_NE(s.find("Block #2"), std::string::npos);
    EXPECT_NE(s.find("test data"), std::string::npos);
    EXPECT_NE(s.find("abcd"), std::string::npos);
}

// ---- Blockchain tests --------------------------------------------------

TEST(BlockchainTest, StartsWithGenesisBlock) {
    Blockchain bc;
    const auto &chain = bc.fetchAll();

    ASSERT_EQ(chain.size(), 1u);
    EXPECT_EQ(chain[0].getIndex(), 0u);
    EXPECT_EQ(chain[0].getData(), "Genesis Block");
}

TEST(BlockchainTest, AddBlockIncreasesChainSize) {
    Blockchain bc;
    bc.addBlock("Block 1");
    bc.addBlock("Block 2");

    EXPECT_EQ(bc.fetchAll().size(), 3u);
}

TEST(BlockchainTest, BlocksAreProperlyChained) {
    Blockchain bc;
    bc.addBlock("Block 1");
    bc.addBlock("Block 2");

    const auto &chain = bc.fetchAll();
    for (size_t i = 1; i < chain.size(); ++i) {
        EXPECT_EQ(chain[i].getPreviousHash(), chain[i - 1].getHash())
            << "Chain broken between blocks " << (i - 1) << " and " << i;
    }
}

TEST(BlockchainTest, BlockIndicesAreSequential) {
    Blockchain bc;
    bc.addBlock("A");
    bc.addBlock("B");
    bc.addBlock("C");

    const auto &chain = bc.fetchAll();
    for (size_t i = 0; i < chain.size(); ++i) {
        EXPECT_EQ(chain[i].getIndex(), static_cast<uint32_t>(i));
    }
}

TEST(BlockchainTest, FetchAllReturnsSameData) {
    Blockchain bc;
    bc.addBlock("hello");

    const auto &chain = bc.fetchAll();
    ASSERT_EQ(chain.size(), 2u);
    EXPECT_EQ(chain[1].getData(), "hello");
}
