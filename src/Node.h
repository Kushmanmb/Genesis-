#pragma once

#include "Blockchain.h"
#include <cstdint>
#include <string>

// A Node owns a Blockchain instance and can be started and stopped.
// While running it accepts block submissions and provides read access
// to the underlying chain.
class Node {
public:
    Node();

    // Start the node.  Idempotent: calling start() on an already-running
    // node is a no-op.
    void start();

    // Stop the node.  Idempotent: calling stop() on an already-stopped
    // node is a no-op.
    void stop();

    // Returns true while the node has been started and not yet stopped.
    [[nodiscard]] bool isRunning() const;

    // Delegate to Blockchain::addBlock (owner-authenticated).
    // Throws std::runtime_error if the node is not running.
    void addBlock(const std::string &data, const std::string &callerAddress);

    // Record the owner's social profile on the underlying chain and return true
    // if it is confirmed.  Throws std::runtime_error if the node is not running.
    [[nodiscard]] bool validateSocialProfile();

    // Broadcast an announcement by recording "Announcement: <message>" as a block
    // on the chain.  callerAddress must be an authorised owner.
    // Throws std::runtime_error if the node is not running.
    void announce(const std::string &message, const std::string &callerAddress);

    // Read-only access to the underlying chain.
    // Throws std::runtime_error if the node is not running.
    [[nodiscard]] const std::vector<Block> &fetchAll() const;

    // Query the Etherscan v2 API for the latest Ethereum mainnet block number.
    // Performs an HTTP GET to:
    //   https://api.etherscan.io/v2/api?chainid=1&module=proxy&action=eth_blockNumber&apikey=<apiKey>
    // Returns the block number as a uint64_t.
    // Throws std::runtime_error on network failure or an unexpected response.
    [[nodiscard]] static uint64_t fetchLatestEthBlockNumber(const std::string &apiKey);

    // Parse an Etherscan eth_blockNumber JSON response and return the block number.
    // Expected format: {"jsonrpc":"2.0","id":83,"result":"0x<hex>"}
    // Throws std::runtime_error if the response is missing the "result" field or
    // contains a malformed hex value.
    [[nodiscard]] static uint64_t parseEthBlockNumberResponse(const std::string &response);

    // Query the Etherscan v2 API for the ETH balance of an address on Ethereum mainnet.
    // Performs an HTTP GET to:
    //   https://api.etherscan.io/v2/api?chainid=1&module=account&action=balance&address=<address>&tag=latest&apikey=<apiKey>
    // Returns the balance as a decimal string (in wei).
    // Throws std::runtime_error on network failure or an unexpected response.
    [[nodiscard]] static std::string fetchEthBalance(const std::string &address,
                                                     const std::string &apiKey);

    // Parse an Etherscan account balance JSON response and return the balance (in wei)
    // as a decimal string.
    // Expected format: {"status":"1","message":"OK","result":"<decimal>"}
    // Throws std::runtime_error if the response is missing the "result" field or
    // the result value is empty.
    [[nodiscard]] static std::string parseEthBalanceResponse(const std::string &response);

    // Query the Etherscan v2 API for the total supply of an ERC-20 token on Ethereum mainnet.
    // Performs an HTTP GET to:
    //   https://api.etherscan.io/v2/api?chainid=1&module=stats&action=tokensupply&contractaddress=<contractAddress>&apikey=<apiKey>
    // Returns the token supply as a decimal string.
    // Throws std::runtime_error on network failure or an unexpected response.
    [[nodiscard]] static std::string fetchTokenSupply(const std::string &contractAddress,
                                                      const std::string &apiKey);

    // Parse an Etherscan token supply JSON response and return the supply as a decimal string.
    // Expected format: {"status":"1","message":"OK","result":"<decimal>"}
    // Throws std::runtime_error if the response is missing the "result" field or
    // the result value is empty.
    [[nodiscard]] static std::string parseTokenSupplyResponse(const std::string &response);

private:
    Blockchain blockchain;
    bool       running;
};
