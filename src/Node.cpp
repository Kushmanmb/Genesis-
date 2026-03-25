#include "Node.h"

#include <stdexcept>

Node::Node() : running(false) {}

void Node::start() {
    running = true;
}

void Node::stop() {
    running = false;
}

bool Node::isRunning() const {
    return running;
}

void Node::addBlock(const std::string &data, const std::string &callerAddress) {
    if (!running) {
        throw std::runtime_error("Node is not running");
    }
    blockchain.addBlock(data, callerAddress);
}

const std::vector<Block> &Node::fetchAll() const {
    if (!running) {
        throw std::runtime_error("Node is not running");
    }
    return blockchain.fetchAll();
}
