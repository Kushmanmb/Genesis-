#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>
#include <string>
#include <string_view>

// Addresses that hold owner-level permissions on this blockchain.
// No contributors are currently configured; all owner-gated operations will be rejected.
inline constexpr std::array<std::string_view, 0> OWNER_ADDRESSES = {};

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

// Returns true when `address` matches one of the configured owner addresses
// (case-sensitive, as Ethereum checksummed addresses are case-sensitive).
inline bool isOwner(const std::string &address) {
    return std::find(OWNER_ADDRESSES.begin(), OWNER_ADDRESSES.end(), address)
           != OWNER_ADDRESSES.end();
}
