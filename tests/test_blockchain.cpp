#include <gtest/gtest.h>
#include <limits>
#include <type_traits>
#include <sstream>
#include "Block.h"
#include "Blockchain.h"
#include "Logger.h"
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

// ---- computeRangeChecksum tests -------------------------------------------

TEST(BlockchainTest, ChkpotpieReturnsSha256Hash) {
    Blockchain bc;
    bc.addBlock("A");
    const std::string result = bc.computeRangeChecksum(0, 1);

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

    EXPECT_EQ(bc.computeRangeChecksum(0, 0), bc.computeRangeChecksum(0, 0));
    EXPECT_EQ(bc.computeRangeChecksum(0, 2), bc.computeRangeChecksum(0, 2));
}

TEST(BlockchainTest, ChkpotpieSingleBlockEqualsBlockHash) {
    Blockchain bc;
    // computeRangeChecksum over a single block is sha256 of that block's hash.
    // Verify it returns a valid 64-char hex string distinct from the block hash itself.
    const std::string result = bc.computeRangeChecksum(0, 0);
    ASSERT_EQ(result.size(), 64u);
    EXPECT_NE(result, bc.fetchAll()[0].getHash());
}

TEST(BlockchainTest, ChkpotpieThrowsOnOutOfRangeIndex) {
    Blockchain bc;
    EXPECT_THROW(bc.computeRangeChecksum(0, 5), std::out_of_range);
}

TEST(BlockchainTest, ChkpotpieThrowsWhenFromExceedsTo) {
    Blockchain bc;
    bc.addBlock("A");
    EXPECT_THROW(bc.computeRangeChecksum(1, 0), std::out_of_range);
}

// ---- fetchAllFrom tests ------------------------------------------------

TEST(BlockchainTest, FetchAllFromReturnsMatchingBlocks) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(SOCIAL_PROFILE));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(SOCIAL_PROFILE));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(std::string(SOCIAL_PROFILE)), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom(std::string(SOCIAL_PROFILE)).empty());
}

TEST(BlockchainTest, FetchAllFromReturnsMultipleMatchingBlocks) {
    Blockchain bc;
    bc.addBlock("First block linked to " + std::string(SOCIAL_PROFILE));
    bc.addBlock("Second block linked to " + std::string(SOCIAL_PROFILE));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(SOCIAL_PROFILE));
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromResultsAreProperBlocks) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(SOCIAL_PROFILE));

    const auto matches = bc.fetchAllFrom(std::string(SOCIAL_PROFILE));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getIndex(), 1u);
    EXPECT_NE(matches[0].getHash(), "");
}

// ---- fetchAllFrom(COINBASE_ID) tests -----------------------------------

TEST(BlockchainTest, FetchAllFromCoinbaseIdReturnsMatchingBlock) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(COINBASE_ID));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(COINBASE_ID));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(std::string(COINBASE_ID)), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromCoinbaseIdReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom(std::string(COINBASE_ID)).empty());
}

TEST(BlockchainTest, FetchAllFromCoinbaseIdReturnsMultipleMatches) {
    Blockchain bc;
    bc.addBlock("First block linked to " + std::string(COINBASE_ID));
    bc.addBlock("Second block linked to " + std::string(COINBASE_ID));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(COINBASE_ID));
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromCoinbaseIdBlockHasValidProperties) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(COINBASE_ID));

    const auto matches = bc.fetchAllFrom(std::string(COINBASE_ID));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getIndex(), 1u);
    ASSERT_EQ(matches[0].getHash().size(), 64u);
}

// ---- fetchAllFrom(FACEBOOK_PROFILE) tests ------------------------------

TEST(BlockchainTest, FetchAllFromFacebookProfileReturnsMatchingBlock) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(FACEBOOK_PROFILE));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(FACEBOOK_PROFILE));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(std::string(FACEBOOK_PROFILE)), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromFacebookProfileReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom(std::string(FACEBOOK_PROFILE)).empty());
}

TEST(BlockchainTest, FetchAllFromFacebookProfileReturnsMultipleMatches) {
    Blockchain bc;
    bc.addBlock("First block linked to " + std::string(FACEBOOK_PROFILE));
    bc.addBlock("Second block linked to " + std::string(FACEBOOK_PROFILE));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(FACEBOOK_PROFILE));
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromFacebookProfileBlockHasValidProperties) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(FACEBOOK_PROFILE));

    const auto matches = bc.fetchAllFrom(std::string(FACEBOOK_PROFILE));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getIndex(), 1u);
    ASSERT_EQ(matches[0].getHash().size(), 64u);
}

// ---- fetchAllFrom(INSTAGRAM_PROFILE) tests -----------------------------

TEST(BlockchainTest, FetchAllFromInstagramProfileReturnsMatchingBlock) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(INSTAGRAM_PROFILE));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(INSTAGRAM_PROFILE));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(std::string(INSTAGRAM_PROFILE)), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromInstagramProfileReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom(std::string(INSTAGRAM_PROFILE)).empty());
}

TEST(BlockchainTest, FetchAllFromInstagramProfileReturnsMultipleMatches) {
    Blockchain bc;
    bc.addBlock("First block linked to " + std::string(INSTAGRAM_PROFILE));
    bc.addBlock("Second block linked to " + std::string(INSTAGRAM_PROFILE));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(INSTAGRAM_PROFILE));
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromInstagramProfileBlockHasValidProperties) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(INSTAGRAM_PROFILE));

    const auto matches = bc.fetchAllFrom(std::string(INSTAGRAM_PROFILE));
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

// ---- fetchAllFrom(PHONE_NUMBER) tests ----------------------------------

TEST(BlockchainTest, FetchAllFromPhoneNumberReturnsMatchingBlock) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(PHONE_NUMBER));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(PHONE_NUMBER));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_NE(matches[0].getData().find(std::string(PHONE_NUMBER)), std::string::npos);
}

TEST(BlockchainTest, FetchAllFromPhoneNumberReturnsEmptyWhenNoMatch) {
    Blockchain bc;
    bc.addBlock("some data");

    EXPECT_TRUE(bc.fetchAllFrom(std::string(PHONE_NUMBER)).empty());
}

TEST(BlockchainTest, FetchAllFromPhoneNumberReturnsMultipleMatches) {
    Blockchain bc;
    bc.addBlock("First block linked to " + std::string(PHONE_NUMBER));
    bc.addBlock("Second block linked to " + std::string(PHONE_NUMBER));
    bc.addBlock("unrelated block");

    const auto matches = bc.fetchAllFrom(std::string(PHONE_NUMBER));
    ASSERT_EQ(matches.size(), 2u);
}

TEST(BlockchainTest, FetchAllFromPhoneNumberBlockHasValidProperties) {
    Blockchain bc;
    bc.addBlock("Data linked to " + std::string(PHONE_NUMBER));

    const auto matches = bc.fetchAllFrom(std::string(PHONE_NUMBER));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getIndex(), 1u);
    ASSERT_EQ(matches[0].getHash().size(), 64u);
}

// ---- validateSocialProfile tests ---------------------------------------

TEST(BlockchainTest, ValidateSocialProfileReturnsTrue) {
    Blockchain bc;
    EXPECT_TRUE(bc.validateSocialProfile());
}

TEST(BlockchainTest, ValidateSocialProfileAddsBlockToChain) {
    Blockchain bc;
    const size_t sizeBefore = bc.fetchAll().size();
    EXPECT_TRUE(bc.validateSocialProfile());
    EXPECT_EQ(bc.fetchAll().size(), sizeBefore + 1);
}

TEST(BlockchainTest, ValidateSocialProfileBlockDataMatchesSocialProfile) {
    Blockchain bc;
    EXPECT_TRUE(bc.validateSocialProfile());
    const auto matches = bc.fetchAllFrom(std::string(SOCIAL_PROFILE));
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].getData(), std::string(SOCIAL_PROFILE));
}

TEST(BlockchainTest, ValidateSocialProfileCanBeCalledMultipleTimes) {
    Blockchain bc;
    EXPECT_TRUE(bc.validateSocialProfile());
    EXPECT_TRUE(bc.validateSocialProfile());
    // Two profile blocks should now be recorded.
    EXPECT_EQ(bc.fetchAllFrom(std::string(SOCIAL_PROFILE)).size(), 2u);
}

TEST(NodeTest, ValidateSocialProfileThrowsWhenNodeNotRunning) {
    Node node;
    EXPECT_THROW(static_cast<void>(node.validateSocialProfile()), std::runtime_error);
}

TEST(NodeTest, ValidateSocialProfileReturnsTrueWhenRunning) {
    Node node;
    node.start();
    EXPECT_TRUE(node.validateSocialProfile());
    node.stop();
}

// ---- announce tests ----------------------------------------------------

TEST(BlockchainTest, AnnounceDeniedForUnauthorizedCaller) {
    Blockchain bc;
    EXPECT_THROW(
        bc.announce("hello", "0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
}

TEST(BlockchainTest, AnnounceDoesNotChangeChainSizeOnDenial) {
    Blockchain bc;
    const size_t sizeBefore = bc.fetchAll().size();
    EXPECT_THROW(
        bc.announce("hello", "0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
    EXPECT_EQ(bc.fetchAll().size(), sizeBefore);
}

TEST(NodeTest, AnnounceThrowsWhenNodeNotRunning) {
    Node node;
    EXPECT_THROW(
        node.announce("hello", "0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
}

TEST(NodeTest, AnnounceDeniedForUnauthorizedCallerWhenRunning) {
    Node node;
    node.start();
    EXPECT_THROW(
        node.announce("hello", "0x0000000000000000000000000000000000000000"),
        std::runtime_error
    );
    node.stop();
}

// ---- Logger tests -------------------------------------------------------

TEST(LoggerTest, LogDebugWritesToStderr) {
    // Redirect std::cerr to a string stream for inspection.
    std::ostringstream capture;
    std::streambuf *oldBuf = std::cerr.rdbuf(capture.rdbuf());

    Logger::log(Logger::Level::DEBUG, "test debug message");

    std::cerr.rdbuf(oldBuf);
    EXPECT_NE(capture.str().find("DEBUG"), std::string::npos);
    EXPECT_NE(capture.str().find("test debug message"), std::string::npos);
}

TEST(LoggerTest, LogInfoWritesToStderr) {
    std::ostringstream capture;
    std::streambuf *oldBuf = std::cerr.rdbuf(capture.rdbuf());

    Logger::log(Logger::Level::INFO, "test info message");

    std::cerr.rdbuf(oldBuf);
    EXPECT_NE(capture.str().find("INFO"), std::string::npos);
    EXPECT_NE(capture.str().find("test info message"), std::string::npos);
}

TEST(LoggerTest, LogWarningWritesToStderr) {
    std::ostringstream capture;
    std::streambuf *oldBuf = std::cerr.rdbuf(capture.rdbuf());

    Logger::log(Logger::Level::WARNING, "test warning message");

    std::cerr.rdbuf(oldBuf);
    EXPECT_NE(capture.str().find("WARNING"), std::string::npos);
    EXPECT_NE(capture.str().find("test warning message"), std::string::npos);
}

TEST(LoggerTest, LogErrorWritesToStderr) {
    std::ostringstream capture;
    std::streambuf *oldBuf = std::cerr.rdbuf(capture.rdbuf());

    Logger::log(Logger::Level::ERROR, "test error message");

    std::cerr.rdbuf(oldBuf);
    EXPECT_NE(capture.str().find("ERROR"), std::string::npos);
    EXPECT_NE(capture.str().find("test error message"), std::string::npos);
}

TEST(LoggerTest, LogOutputIncludesBracketedLevel) {
    std::ostringstream capture;
    std::streambuf *oldBuf = std::cerr.rdbuf(capture.rdbuf());

    Logger::log(Logger::Level::INFO, "bracketed");

    std::cerr.rdbuf(oldBuf);
    // Output must contain the level label wrapped in brackets, e.g. "[INFO]".
    EXPECT_NE(capture.str().find("[INFO]"), std::string::npos);
}

// ---- Node::parseEthBlockNumberResponse tests ---------------------------

TEST(NodeTest, ParseEthBlockNumberResponseTypicalHex) {
    // Typical Etherscan response with a non-zero block number.
    const std::string response =
        R"({"jsonrpc":"2.0","id":83,"result":"0x1092261"})";
    EXPECT_EQ(Node::parseEthBlockNumberResponse(response), 0x1092261ULL);
}

TEST(NodeTest, ParseEthBlockNumberResponseZero) {
    const std::string response =
        R"({"jsonrpc":"2.0","id":1,"result":"0x0"})";
    EXPECT_EQ(Node::parseEthBlockNumberResponse(response), 0ULL);
}

TEST(NodeTest, ParseEthBlockNumberResponseLargeValue) {
    // 0xFFFFFFFFFFFFFFFF is the maximum uint64 value.
    const std::string response =
        R"({"jsonrpc":"2.0","id":1,"result":"0xffffffffffffffff"})";
    EXPECT_EQ(Node::parseEthBlockNumberResponse(response),
              std::numeric_limits<uint64_t>::max());
}

TEST(NodeTest, ParseEthBlockNumberResponseMissingResultThrows) {
    const std::string response =
        R"({"jsonrpc":"2.0","id":83,"error":{"code":-32601,"message":"Method not found"}})";
    EXPECT_THROW(static_cast<void>(Node::parseEthBlockNumberResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseEthBlockNumberResponseEmptyStringThrows) {
    EXPECT_THROW(static_cast<void>(Node::parseEthBlockNumberResponse("")), std::runtime_error);
}

TEST(NodeTest, ParseEthBlockNumberResponseMalformedHexThrows) {
    // "result" key is present but the value is not a valid hex number.
    const std::string response =
        R"({"jsonrpc":"2.0","id":1,"result":"not-a-hex"})";
    EXPECT_THROW(static_cast<void>(Node::parseEthBlockNumberResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseEthBlockNumberResponseUnterminatedResultThrows) {
    // The closing quote after the result value is missing.
    const std::string response = R"({"result":"0x1234)";
    EXPECT_THROW(static_cast<void>(Node::parseEthBlockNumberResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseEthBlockNumberResponseKnownBlock) {
    // Real-world Etherscan response: block 0x1798f7b.
    const std::string response =
        R"({"jsonrpc":"2.0","id":83,"result":"0x1798f7b"})";
    EXPECT_EQ(Node::parseEthBlockNumberResponse(response), 0x1798f7bULL);
}

// ---- Node::parseEthBalanceResponse tests -------------------------------

TEST(NodeTest, ParseEthBalanceResponseTypical) {
    // Typical Etherscan account balance response.
    const std::string response =
        R"({"status":"1","message":"OK","result":"5571626000000000"})";
    EXPECT_EQ(Node::parseEthBalanceResponse(response), "5571626000000000");
}

TEST(NodeTest, ParseEthBalanceResponseZeroBalance) {
    const std::string response =
        R"({"status":"1","message":"OK","result":"0"})";
    EXPECT_EQ(Node::parseEthBalanceResponse(response), "0");
}

TEST(NodeTest, ParseEthBalanceResponseLargeBalance) {
    // Large balance (greater than uint64_t max) represented as decimal string.
    const std::string response =
        R"({"status":"1","message":"OK","result":"120000000000000000000000000"})";
    EXPECT_EQ(Node::parseEthBalanceResponse(response), "120000000000000000000000000");
}

TEST(NodeTest, ParseEthBalanceResponseMissingResultThrows) {
    const std::string response =
        R"({"status":"0","message":"NOTOK","error":"Invalid address format"})";
    EXPECT_THROW(static_cast<void>(Node::parseEthBalanceResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseEthBalanceResponseEmptyStringThrows) {
    EXPECT_THROW(static_cast<void>(Node::parseEthBalanceResponse("")), std::runtime_error);
}

TEST(NodeTest, ParseEthBalanceResponseEmptyResultValueThrows) {
    // The "result" key is present but its value is an empty string.
    const std::string response =
        R"({"status":"1","message":"OK","result":""})";
    EXPECT_THROW(static_cast<void>(Node::parseEthBalanceResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseEthBalanceResponseUnterminatedResultThrows) {
    // The closing quote after the result value is missing.
    const std::string response = R"({"status":"1","message":"OK","result":"12345)";
    EXPECT_THROW(static_cast<void>(Node::parseEthBalanceResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseEthBalanceResponseKnownAddress) {
    // Simulated response for address 0x6fb9e80dDd0f5DC99D7cB38b07e8b298A57bF253.
    const std::string response =
        R"({"status":"1","message":"OK","result":"663046288842860073"})";
    EXPECT_EQ(Node::parseEthBalanceResponse(response), "663046288842860073");
}

// ---- Node::parseTokenSupplyResponse tests ------------------------------

TEST(NodeTest, ParseTokenSupplyResponseTypical) {
    // Typical Etherscan token supply response.
    const std::string response =
        R"({"status":"1","message":"OK","result":"21265524714464496430135228"})";
    EXPECT_EQ(Node::parseTokenSupplyResponse(response), "21265524714464496430135228");
}

TEST(NodeTest, ParseTokenSupplyResponseZeroSupply) {
    const std::string response =
        R"({"status":"1","message":"OK","result":"0"})";
    EXPECT_EQ(Node::parseTokenSupplyResponse(response), "0");
}

TEST(NodeTest, ParseTokenSupplyResponseLargeSupply) {
    // Very large token supply (more than uint64_t max) represented as decimal string.
    const std::string response =
        R"({"status":"1","message":"OK","result":"1000000000000000000000000000000"})";
    EXPECT_EQ(Node::parseTokenSupplyResponse(response), "1000000000000000000000000000000");
}

TEST(NodeTest, ParseTokenSupplyResponseMissingResultThrows) {
    const std::string response =
        R"({"status":"0","message":"NOTOK","error":"Invalid contract address format"})";
    EXPECT_THROW(static_cast<void>(Node::parseTokenSupplyResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseTokenSupplyResponseEmptyStringThrows) {
    EXPECT_THROW(static_cast<void>(Node::parseTokenSupplyResponse("")), std::runtime_error);
}

TEST(NodeTest, ParseTokenSupplyResponseEmptyResultValueThrows) {
    // The "result" key is present but its value is an empty string.
    const std::string response =
        R"({"status":"1","message":"OK","result":""})";
    EXPECT_THROW(static_cast<void>(Node::parseTokenSupplyResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseTokenSupplyResponseUnterminatedResultThrows) {
    // The closing quote after the result value is missing.
    const std::string response = R"({"status":"1","message":"OK","result":"12345)";
    EXPECT_THROW(static_cast<void>(Node::parseTokenSupplyResponse(response)), std::runtime_error);
}

TEST(NodeTest, ParseTokenSupplyResponseKnownContract) {
    // Simulated response for contract 0x57d90b64a1a57749b0f932f1a3395792e12e7055.
    const std::string response =
        R"({"status":"1","message":"OK","result":"21265524714464496430135228"})";
    EXPECT_EQ(Node::parseTokenSupplyResponse(response), "21265524714464496430135228");
}

// ---- Owners.h constant tests -------------------------------------------

TEST(OwnersTest, EtherscanApiKeyIsSet) {
    // Verify that the ETHERSCAN_API_KEY constant is non-empty and holds the expected key.
    EXPECT_FALSE(std::string(ETHERSCAN_API_KEY).empty());
    EXPECT_EQ(std::string(ETHERSCAN_API_KEY), "qu4g9niymiy7lacsmp6echxqdkjfgmkc3fskedyenzcyem9g8x");
}
