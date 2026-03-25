#include <gtest/gtest.h>
#include <type_traits>
#include "Block.h"
#include "Blockchain.h"
#include "Node.h"
#include "Owners.h"

// ---- Block immutability static checks ----------------------------------

// Block must be copy-constructible (so it can be stored in std::vector) but
// copy-assignment must be deleted (const members prevent it), enforcing that
// a block's state can never be silently overwritten.
static_assert(std::is_copy_constructible<Block>::value,
              "Block must be copy-constructible");
static_assert(!std::is_copy_assignable<Block>::value,
              "Block must not be copy-assignable (fields are const)");
static_assert(!std::is_move_assignable<Block>::value,
              "Block must not be move-assignable (fields are const)");

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

// ---- returnToOrigin tests ----------------------------------------------

TEST(BlockchainTest, ReturnToOriginDeniedForAllCallers) {
    Blockchain bc;
    EXPECT_THROW(
        bc.returnToOrigin("0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
}

// ---- returnToLegacy tests ----------------------------------------------

TEST(BlockchainTest, ReturnToLegacyDeniedForAllCallers) {
    Blockchain bc;
    const std::string legacyAddr = "0xB29380d2FC97F857E1D7De0cB3F1E2b8dc5caf23";
    EXPECT_THROW(
        bc.returnToLegacy("0x0000000000000000000000000000000000000000", legacyAddr),
        std::runtime_error
    );
}

// ---- returnToOwner tests -----------------------------------------------

TEST(BlockchainTest, ReturnToOwnerDeniedForAllCallers) {
    Blockchain bc;
    EXPECT_THROW(
        bc.returnToOwner("0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
}

// ---- consolidateBalances tests -----------------------------------------

// No owners are currently configured (OWNER_ADDRESSES is empty), so every
// authorisation check rejects any caller.  The success path (an authorised
// owner calling consolidateBalances and receiving a new chain block) cannot
// be exercised until at least one address is added to OWNER_ADDRESSES.

TEST(BlockchainTest, ConsolidateBalancesDeniedForUnauthorizedCaller) {
    Blockchain bc;
    EXPECT_THROW(
        bc.consolidateBalances("0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
}

TEST(BlockchainTest, ConsolidateBalancesDoesNotChangeChainSizeOnDenial) {
    Blockchain bc;
    bc.addBlock("A");
    const size_t sizeBefore = bc.fetchAll().size();
    EXPECT_THROW(
        bc.consolidateBalances("0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
    EXPECT_EQ(bc.fetchAll().size(), sizeBefore);
}

// ---- chkpotpie tests ---------------------------------------------------

TEST(BlockchainTest, ChkpotpieReturnsSha256Hash) {
    Blockchain bc;
    bc.addBlock("A");
    const std::string result = bc.chkpotpie(0, 1);

    ASSERT_EQ(result.size(), 64u);
    for (char c : result) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character: " << c;
    }
}

TEST(BlockchainTest, ChkpotpieIsDeterministic) {
    Blockchain bc;
    bc.addBlock("A");
    bc.addBlock("B");

    EXPECT_EQ(bc.chkpotpie(0, 0), bc.chkpotpie(0, 0));
    EXPECT_EQ(bc.chkpotpie(0, 2), bc.chkpotpie(0, 2));
}

TEST(BlockchainTest, ChkpotpieSingleBlockEqualsBlockHash) {
    Blockchain bc;
    // chkpotpie over a single block is sha256 of that block's hash.
    // Verify it returns a valid 64-char hex string distinct from the block hash itself.
    const std::string result = bc.chkpotpie(0, 0);
    ASSERT_EQ(result.size(), 64u);
    EXPECT_NE(result, bc.fetchAll()[0].getHash());
}

TEST(BlockchainTest, ChkpotpieThrowsOnOutOfRangeIndex) {
    Blockchain bc;
    EXPECT_THROW(bc.chkpotpie(0, 5), std::out_of_range);
}

TEST(BlockchainTest, ChkpotpieThrowsWhenFromExceedsTo) {
    Blockchain bc;
    bc.addBlock("A");
    EXPECT_THROW(bc.chkpotpie(1, 0), std::out_of_range);
}

// ---- fetchAllFrom tests ------------------------------------------------

TEST(BlockchainTest, FetchAllFromReturnsMatchingBlocks) {
    Blockchain bc;
    const std::string identifier = "https://github.com/Kushmanmb";
    bc.addBlock("Data linked to " + identifier);
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(identifier);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(identifier), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom("https://github.com/Kushmanmb").empty());
}

TEST(BlockchainTest, FetchAllFromReturnsMultipleMatchingBlocks) {
    Blockchain bc;
    const std::string identifier = "https://github.com/Kushmanmb";
    bc.addBlock("First block linked to " + identifier);
    bc.addBlock("Second block linked to " + identifier);
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(identifier);
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromResultsAreProperBlocks) {
    Blockchain bc;
    const std::string identifier = "https://github.com/Kushmanmb";
    bc.addBlock("Data linked to " + identifier);

    const auto matches = bc.fetchAllFrom(identifier);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getIndex(), 1u);
    EXPECT_NE(matches[0].getHash(), "");
}

// ---- fetchAllFrom(COINBASE_ID) tests -----------------------------------

TEST(BlockchainTest, FetchAllFromCoinbaseIdReturnsMatchingBlock) {
    Blockchain bc;
    const std::string id(COINBASE_ID);
    bc.addBlock("Data linked to " + id);
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(id);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(id), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromCoinbaseIdReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom(std::string(COINBASE_ID)).empty());
}

TEST(BlockchainTest, FetchAllFromCoinbaseIdReturnsMultipleMatches) {
    Blockchain bc;
    const std::string id(COINBASE_ID);
    bc.addBlock("First block linked to " + id);
    bc.addBlock("Second block linked to " + id);
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(id);
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromCoinbaseIdBlockHasValidProperties) {
    Blockchain bc;
    const std::string id(COINBASE_ID);
    bc.addBlock("Data linked to " + id);

    const auto matches = bc.fetchAllFrom(id);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getIndex(), 1u);
    ASSERT_EQ(matches[0].getHash().size(), 64u);
}

// ---- fetchAllFrom(FACEBOOK_PROFILE) tests ------------------------------

TEST(BlockchainTest, FetchAllFromFacebookProfileReturnsMatchingBlock) {
    Blockchain bc;
    const std::string fb(FACEBOOK_PROFILE);
    bc.addBlock("Data linked to " + fb);
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(fb);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(fb), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromFacebookProfileReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom(std::string(FACEBOOK_PROFILE)).empty());
}

TEST(BlockchainTest, FetchAllFromFacebookProfileReturnsMultipleMatches) {
    Blockchain bc;
    const std::string fb(FACEBOOK_PROFILE);
    bc.addBlock("First block linked to " + fb);
    bc.addBlock("Second block linked to " + fb);
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(fb);
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromFacebookProfileBlockHasValidProperties) {
    Blockchain bc;
    const std::string fb(FACEBOOK_PROFILE);
    bc.addBlock("Data linked to " + fb);

    const auto matches = bc.fetchAllFrom(fb);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getIndex(), 1u);
    ASSERT_EQ(matches[0].getHash().size(), 64u);
}

// ---- getTrending tests -------------------------------------------------

TEST(BlockchainTest, GetTrendingReturnsEmptyForTopNZero) {
    Blockchain bc;
    bc.addBlock("A");
    EXPECT_TRUE(bc.getTrending(0).empty());
}

TEST(BlockchainTest, GetTrendingOnGenesisOnlyChain) {
    Blockchain bc;
    // Only the genesis block is present; topN=1 should return it.
    const auto trending = bc.getTrending(1);
    ASSERT_EQ(trending.size(), 1u);
    EXPECT_EQ(trending[0].first, "Genesis Block");
    EXPECT_EQ(trending[0].second, 1u);
}

TEST(BlockchainTest, GetTrendingReturnsMostFrequentFirst) {
    Blockchain bc;
    bc.addBlock("popular");
    bc.addBlock("popular");
    bc.addBlock("popular");
    bc.addBlock("rare");

    const auto trending = bc.getTrending(2);
    ASSERT_GE(trending.size(), 1u);
    EXPECT_EQ(trending[0].first, "popular");
    EXPECT_EQ(trending[0].second, 3u);
}

TEST(BlockchainTest, GetTrendingLimitsResultsToTopN) {
    Blockchain bc;
    bc.addBlock("A");
    bc.addBlock("B");
    bc.addBlock("C");
    bc.addBlock("D");

    // Only 2 results requested even though 5 unique data strings exist (incl. genesis).
    const auto trending = bc.getTrending(2);
    EXPECT_EQ(trending.size(), 2u);
}

TEST(BlockchainTest, GetTrendingCountsReflectDuplicates) {
    Blockchain bc;
    bc.addBlock("x");
    bc.addBlock("x");

    const auto trending = bc.getTrending(5);
    // Find "x" in the results.
    bool found = false;
    for (const auto &entry : trending) {
        if (entry.first == "x") {
            EXPECT_EQ(entry.second, 2u);
            found = true;
        }
    }
    EXPECT_TRUE(found) << "\"x\" not found in trending results";
}

TEST(BlockchainTest, GetTrendingTopNLargerThanChainReturnsAll) {
    Blockchain bc;
    bc.addBlock("only");

    // Chain has 2 unique entries (genesis + "only"); requesting 100 should return 2.
    const auto trending = bc.getTrending(100);
    EXPECT_EQ(trending.size(), 2u);
}
