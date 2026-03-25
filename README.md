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
- `fetchAll()` — returns a const reference to the full vector of blocks.
