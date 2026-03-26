#include "Node.h"
#include "Logger.h"

#include <curl/curl.h>
#include <memory>
#include <stdexcept>

// libcurl write callback — appends received data to an std::string.
static std::size_t curlWriteCallback(char *ptr, std::size_t size,
                                     std::size_t nmemb, std::string *out) {
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

Node::Node() : running(false) {}

void Node::start() {
    if (!running) {
        running = true;
        LOG_DEBUG("Node started");
    }
}

void Node::stop() {
    if (running) {
        running = false;
        LOG_DEBUG("Node stopped");
    }
}

bool Node::isRunning() const {
    return running;
}

void Node::addBlock(const std::string &data, const std::string &callerAddress) {
    if (!running) {
        throw std::runtime_error("Node is not running");
    }
    LOG_DEBUG("Node::addBlock data=" + data + " caller=" + callerAddress);
    blockchain.addBlock(data, callerAddress);
}

const std::vector<Block> &Node::fetchAll() const {
    if (!running) {
        throw std::runtime_error("Node is not running");
    }
    LOG_DEBUG("Node::fetchAll");
    return blockchain.fetchAll();
}

bool Node::validateSocialProfile() {
    if (!running) {
        throw std::runtime_error("Node is not running");
    }
    LOG_DEBUG("Node::validateSocialProfile");
    return blockchain.validateSocialProfile();
}

void Node::announce(const std::string &message, const std::string &callerAddress) {
    if (!running) {
        throw std::runtime_error("Node is not running");
    }
    LOG_DEBUG("Node::announce message=" + message + " caller=" + callerAddress);
    blockchain.announce(message, callerAddress);
}

uint64_t Node::fetchLatestEthBlockNumber(const std::string &apiKey) {
    const std::string url =
        "https://api.etherscan.io/v2/api"
        "?chainid=1&module=proxy&action=eth_blockNumber&apikey=" + apiKey;

    // RAII wrapper ensures curl_easy_cleanup is called even if an exception is thrown.
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
        curl_easy_init(), curl_easy_cleanup);
    if (!curl) {
        throw std::runtime_error("fetchLatestEthBlockNumber: failed to initialize libcurl");
    }

    std::string response;
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 10L);

    const CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        throw std::runtime_error(
            std::string("fetchLatestEthBlockNumber: HTTP request failed: ") +
            curl_easy_strerror(res));
    }

    LOG_DEBUG("fetchLatestEthBlockNumber response=" + response);
    return parseEthBlockNumberResponse(response);
}

uint64_t Node::parseEthBlockNumberResponse(const std::string &response) {
    // Expected: {"jsonrpc":"2.0","id":83,"result":"0x<hex>"}
    const std::string resultKey = "\"result\":\"";
    const auto keyPos = response.find(resultKey);
    if (keyPos == std::string::npos) {
        throw std::runtime_error(
            "parseEthBlockNumberResponse: missing \"result\" field in response: " + response);
    }

    const auto hexStart = keyPos + resultKey.size();
    const auto hexEnd   = response.find('"', hexStart);
    if (hexEnd == std::string::npos) {
        throw std::runtime_error(
            "parseEthBlockNumberResponse: malformed response (unterminated result): " + response);
    }

    const std::string hexStr = response.substr(hexStart, hexEnd - hexStart);
    if (hexStr.empty()) {
        throw std::runtime_error(
            "parseEthBlockNumberResponse: empty result value in response: " + response);
    }

    try {
        return std::stoull(hexStr, nullptr, 16);
    } catch (const std::exception &e) {
        throw std::runtime_error(
            std::string("parseEthBlockNumberResponse: failed to parse block number \"") +
            hexStr + "\": " + e.what());
    }
}

std::string Node::fetchEthBalance(const std::string &address, const std::string &apiKey) {
    const std::string url =
        "https://api.etherscan.io/v2/api"
        "?chainid=1&module=account&action=balance&address=" + address +
        "&tag=latest&apikey=" + apiKey;

    // RAII wrapper ensures curl_easy_cleanup is called even if an exception is thrown.
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
        curl_easy_init(), curl_easy_cleanup);
    if (!curl) {
        throw std::runtime_error("fetchEthBalance: failed to initialize libcurl");
    }

    std::string response;
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 10L);

    const CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        throw std::runtime_error(
            std::string("fetchEthBalance: HTTP request failed: ") +
            curl_easy_strerror(res));
    }

    LOG_DEBUG("fetchEthBalance response=" + response);
    return parseEthBalanceResponse(response);
}

std::string Node::parseEthBalanceResponse(const std::string &response) {
    // Expected: {"status":"1","message":"OK","result":"<decimal>"}
    const std::string resultKey = "\"result\":\"";
    const auto keyPos = response.find(resultKey);
    if (keyPos == std::string::npos) {
        throw std::runtime_error(
            "parseEthBalanceResponse: missing \"result\" field in response: " + response);
    }

    const auto valStart = keyPos + resultKey.size();
    const auto valEnd   = response.find('"', valStart);
    if (valEnd == std::string::npos) {
        throw std::runtime_error(
            "parseEthBalanceResponse: malformed response (unterminated result): " + response);
    }

    const std::string balance = response.substr(valStart, valEnd - valStart);
    if (balance.empty()) {
        throw std::runtime_error(
            "parseEthBalanceResponse: empty result value in response: " + response);
    }

    return balance;
}
