#pragma once

#include "Blockchain.h"
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
    bool isRunning() const;

    // Delegate to Blockchain::addBlock (owner-authenticated).
    // Throws std::runtime_error if the node is not running.
    void addBlock(const std::string &data, const std::string &callerAddress);

    // Read-only access to the underlying chain.
    // Throws std::runtime_error if the node is not running.
    const std::vector<Block> &fetchAll() const;

private:
    Blockchain blockchain;
    bool       running;
};
