#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

// Addresses that hold owner-level permissions on this blockchain.
// No contributors are currently configured; all owner-gated operations will be rejected.
inline constexpr std::array<std::string_view, 0> OWNER_ADDRESSES = {};

// Social profile associated with the owner of this blockchain.
inline constexpr std::string_view SOCIAL_PROFILE = "https://github.com/ghost";

// Facebook profile associated with the owner of this blockchain.
inline constexpr std::string_view FACEBOOK_PROFILE = "https://www.facebook.com/Kushmanmb23/";

// Instagram profile associated with the owner of this blockchain.
inline constexpr std::string_view INSTAGRAM_PROFILE = "https://www.instagram.com/kushmanmb/";

// Coinbase ID associated with the owner of this blockchain.
inline constexpr std::string_view COINBASE_ID = "kushman.cb.id";

// Phone number associated with the owner of this blockchain.
inline constexpr std::string_view PHONE_NUMBER = "18542123378";

// Etherscan API key used for querying the Etherscan v2 API.
inline constexpr std::string_view ETHERSCAN_API_KEY = "qu4g9niymiy7lacsmp6echxqdkjfgmkc3fskedyenzcyem9g8x";

// Returns true when `address` matches one of the configured owner addresses
// (case-sensitive, as Ethereum checksummed addresses are case-sensitive).
inline bool isOwner(const std::string &address) {
    return std::find(OWNER_ADDRESSES.begin(), OWNER_ADDRESSES.end(), address)
           != OWNER_ADDRESSES.end();
}
