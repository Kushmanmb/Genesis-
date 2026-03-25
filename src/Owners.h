#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

// Addresses that hold owner-level permissions on this blockchain.
inline constexpr std::array<std::string_view, 2> OWNER_ADDRESSES = {
    "0x6fb9e80dDd0f5DC99D7cB38b07e8b298A57bF253",
    "0x0540e1dA908D032D2F74D50C06397cB5f2cbfDdB"
};

// Returns true when `address` matches one of the configured owner addresses
// (case-sensitive, as Ethereum checksummed addresses are case-sensitive).
inline bool isOwner(const std::string &address) {
    return std::find(OWNER_ADDRESSES.begin(), OWNER_ADDRESSES.end(), address)
           != OWNER_ADDRESSES.end();
}
