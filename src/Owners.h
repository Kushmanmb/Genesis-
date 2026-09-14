#pragma once

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

inline std::string trimOwnerAddress(std::string value) {
    const std::string whitespace = " \t\n\r";
    const auto start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

inline std::vector<std::string> parseOwnerAddresses(const std::string &configuredOwners) {
    std::vector<std::string> owners;
    std::stringstream stream(configuredOwners);
    std::string value;
    while (std::getline(stream, value, ',')) {
        value = trimOwnerAddress(value);
        if (!value.empty()) {
            owners.push_back(value);
        }
    }
    return owners;
}

inline constexpr std::string_view DEFAULT_OWNER_ADDRESS = "Yaketh.eth";

inline std::vector<std::string> resolveOwnerAddresses() {
    if (const char *env = std::getenv("OWNER_ADDRESSES")) {
        const std::vector<std::string> owners = parseOwnerAddresses(env);
        if (!owners.empty()) {
            return owners;
        }
    }

    return {std::string(DEFAULT_OWNER_ADDRESS)};
}

// Owner address strings that hold owner-level permissions on this blockchain.
// Values are matched exactly and may be hexadecimal wallet addresses or ENS names.
// The OWNER_ADDRESSES environment variable may override the default with a
// comma-separated list at runtime.
inline std::vector<std::string> ownerAddresses() {
    return resolveOwnerAddresses();
}

// Social profile associated with the owner of this blockchain.
inline constexpr std::string_view SOCIAL_PROFILE = "https://github.com/YOUR_PROFILE";

// Facebook profile associated with the owner of this blockchain.
inline constexpr std::string_view FACEBOOK_PROFILE = "https://www.facebook.com/YOUR_PROFILE";

// Instagram profile associated with the owner of this blockchain.
inline constexpr std::string_view INSTAGRAM_PROFILE = "https://www.instagram.com/YOUR_PROFILE/";

// Coinbase ID associated with the owner of this blockchain.
inline constexpr std::string_view COINBASE_ID = "YOUR_COINBASE_ID";

// Phone number associated with the owner of this blockchain.
inline constexpr std::string_view PHONE_NUMBER = "YOUR_PHONE_NUMBER";

// Etherscan API key used for querying the Etherscan v2 API.
inline constexpr std::string_view ETHERSCAN_API_KEY_PLACEHOLDER = "YOUR_ETHERSCAN_API_KEY";

inline std::string resolveEtherscanApiKey() {
    if (const char *env = std::getenv("ETHERSCAN_API_KEY")) {
        if (*env != '\0') {
            return std::string(env);
        }
    }
    return std::string(ETHERSCAN_API_KEY_PLACEHOLDER);
}

inline const std::string ETHERSCAN_API_KEY = resolveEtherscanApiKey();

// Returns true when `address` exactly matches one of the configured owner address strings.
inline bool isOwner(const std::string &address) {
    const std::vector<std::string> owners = ownerAddresses();
    return std::find(owners.begin(), owners.end(), address) != owners.end();
}
