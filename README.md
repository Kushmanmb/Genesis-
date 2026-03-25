# Genesis-

A minimal blockchain genesis block implementation in C++.

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
./genesis
```

## Output

Running `./genesis` creates a blockchain with a genesis block and two additional blocks, then prints all blocks via `fetchAll()`:

```
=== All Blocks ===
Block #0
  Timestamp  : ...
  Data       : Genesis Block
  Prev Hash  : 0000000000000000000000000000000000000000000000000000000000000000
  Hash       : ...
Block #1
  Timestamp  : ...
  Data       : Block 1 data
  Prev Hash  : ...
  Hash       : ...
Block #2
  Timestamp  : ...
  Data       : Block 2 data
  Prev Hash  : ...
  Hash       : ...
```

## Blockchain API

- `Blockchain()` — constructs a chain seeded with the genesis block.
- `addBlock(data)` — appends a new block linked to the current chain tip.
- `addBlock(data, callerAddress)` — appends a new block; throws if `callerAddress` is not an authorised owner.
- `returnToOrigin(callerAddress)` — records a block transferring all owner tokens back to the origin address; `callerAddress` must be an authorised owner.
- `returnToLegacy(callerAddress, legacyAddress)` — consolidates all mytoken balances held by owner addresses and records a block transferring them to the specified legacy address; `callerAddress` must be an authorised owner.
- `returnToOwner(callerAddress)` — consolidates all mytoken balances held by owner addresses and records a block transferring them back to `callerAddress`; `callerAddress` must be an authorised owner.
- `consolidateBalances(callerAddress)` — consolidates all mytoken balances held by owner addresses into a single chain record without routing them to a specific destination; `callerAddress` must be an authorised owner.
- `fetchAll()` — returns a const reference to the full vector of blocks.
- `fetchAllFrom(identifier)` — returns a copy of all blocks whose data field contains `identifier`.

## Owner Identity Constants (`Owners.h`)

- `OWNER_ADDRESSES` — list of Ethereum addresses with owner-level permissions (currently empty; all owner-gated operations are rejected).
- `SOCIAL_PROFILE` — owner's GitHub profile: `https://github.com/Kushmanmb`.
- `COINBASE_ID` — owner's Coinbase ID: `kushman.cb.id`.

## Immutability

Every `Block` has `const` data members (`index`, `timestamp`, `data`, `previousHash`, `hash`), so its state is fixed at construction time and can never be modified afterwards.  This is enforced at the C++ type-system level:

* `Block` is **copy-constructible** — new blocks can be created from existing ones (required for `std::vector` storage).
* `Block` is **not copy-assignable** and **not move-assignable** — once a block is in the chain it cannot be overwritten.
