#include "Node.h"
#include "Logger.h"

#include <stdexcept>

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
