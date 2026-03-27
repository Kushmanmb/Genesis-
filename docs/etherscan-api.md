# Using the Etherscan API with curl

This document explains how to query the [Etherscan v2 API](https://docs.etherscan.io/) using `curl` from the command line, with a focus on the `addresstokenbalance` endpoint used in this project.

---

## Example Request

```bash
curl "https://api.etherscan.io/v2/api?chainid=1&module=account&action=addresstokenbalance&address=0x983e3660c0bE01991785F80f266A84B911ab59b0&page=1&offset=100&apikey=YourApiKeyToken"
```

Replace `YourApiKeyToken` with your actual Etherscan API key before running the command (see [Obtaining an API Key](#obtaining-an-api-key) below).

---

## URL Parameters

| Parameter   | Value (example)                              | Description                                                                                                      |
|-------------|----------------------------------------------|------------------------------------------------------------------------------------------------------------------|
| `chainid`   | `1`                                          | The EVM chain to query. `1` = Ethereum Mainnet. Other values: `5` (Goerli), `137` (Polygon), etc.               |
| `module`    | `account`                                    | The API module group. `account` exposes address-level queries such as balances and transaction lists.            |
| `action`    | `addresstokenbalance`                        | The specific action within the module. `addresstokenbalance` retrieves all ERC-20 token balances for an address. |
| `address`   | `0x983e3660c0bE01991785F80f266A84B911ab59b0` | The Ethereum address to query. Must be a valid checksummed or lowercase hex address prefixed with `0x`.          |
| `page`      | `1`                                          | Pagination: the page number to return (starts at `1`).                                                           |
| `offset`    | `100`                                        | Pagination: the number of results per page (maximum `100`).                                                      |
| `apikey`    | `YourApiKeyToken`                            | Your Etherscan API key. **Required** for all requests (see below).                                               |

---

## Obtaining an API Key

An API key is **required** by Etherscan to:

- Authenticate your requests and enforce per-key rate limits (currently 5 calls/second on the free tier).
- Prevent abuse and ensure fair usage across all developers.

**Steps to get a free API key:**

1. Go to <https://etherscan.io/register> and create a free account.
2. After logging in, navigate to <https://etherscan.io/myapikey>.
3. Click **Add** to create a new API key. Give it a descriptive label (e.g., `genesis-project`).
4. Copy the generated key and store it securely (treat it like a password — do not commit it to version control).

**Using the key in this project:**

The key is stored in `src/Owners.h` as the compile-time constant `ETHERSCAN_API_KEY` and is passed to the `Node::fetch*` helper functions at runtime. To use your own key, update that constant before building:

```cpp
// src/Owners.h
inline constexpr std::string_view ETHERSCAN_API_KEY = "<your-api-key-here>";
```

> **Warning:** Never commit a real API key to a public repository. Consider loading it from an environment variable or a secrets manager in production builds.

---

## Example Response

A successful response is a JSON object with three top-level fields:

```json
{
  "status": "1",
  "message": "OK",
  "result": [
    {
      "TokenAddress": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
      "TokenName": "USD Coin",
      "TokenSymbol": "USDC",
      "TokenQuantity": "250000000",
      "TokenDivisor": "6"
    },
    {
      "TokenAddress": "0xdac17f958d2ee523a2206206994597c13d831ec7",
      "TokenName": "Tether USD",
      "TokenSymbol": "USDT",
      "TokenQuantity": "100000000",
      "TokenDivisor": "6"
    }
  ]
}
```

### Response Fields

| Field           | Type            | Description                                                                                      |
|-----------------|-----------------|--------------------------------------------------------------------------------------------------|
| `status`        | `"1"` / `"0"`  | `"1"` = success; `"0"` = error (check `message` for details).                                   |
| `message`       | string          | Human-readable status, e.g. `"OK"` or `"No transactions found"`.                                |
| `result`        | array           | List of token balance objects for the queried address.                                           |
| `TokenAddress`  | string          | The ERC-20 contract address for the token.                                                       |
| `TokenName`     | string          | Human-readable token name.                                                                       |
| `TokenSymbol`   | string          | Ticker symbol (e.g. `USDC`).                                                                     |
| `TokenQuantity` | string (integer)| Raw token balance in the token's smallest unit (divide by `10^TokenDivisor` for display amount). |
| `TokenDivisor`  | string (integer)| The number of decimal places the token uses.                                                     |

### Error Response

When the request fails (e.g. invalid address, missing API key), `status` is `"0"`:

```json
{
  "status": "0",
  "message": "NOTOK",
  "result": "Error! Missing or invalid action name"
}
```

---

## Pagination

Use `page` and `offset` together to iterate through large result sets:

```bash
# First 100 tokens
curl "https://api.etherscan.io/v2/api?chainid=1&module=account&action=addresstokenbalance&address=0x983e3660c0bE01991785F80f266A84B911ab59b0&page=1&offset=100&apikey=YourApiKeyToken"

# Next 100 tokens
curl "https://api.etherscan.io/v2/api?chainid=1&module=account&action=addresstokenbalance&address=0x983e3660c0bE01991785F80f266A84B911ab59b0&page=2&offset=100&apikey=YourApiKeyToken"
```

---

## Rate Limits

| Plan  | Calls / second | Calls / day |
|-------|----------------|-------------|
| Free  | 5              | 100,000     |
| Pro   | 20+            | Unlimited   |

If you exceed the rate limit, Etherscan returns a `"Max rate limit reached"` error in the `result` field with `status: "0"`.

---

## References

- [Etherscan API v2 Documentation](https://docs.etherscan.io/)
- [Get Token Account Balance — Etherscan Docs](https://docs.etherscan.io/api-endpoints/accounts#get-token-account-balance-by-contractaddress)
- [Supported Chains — Etherscan Docs](https://docs.etherscan.io/getting-started/supported-chains)
- [API Key Management](https://etherscan.io/myapikey)
- [Etherscan API Plans & Pricing](https://etherscan.io/apis)
