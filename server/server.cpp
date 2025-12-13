#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

#include <zmq.h>

#include "group_manager.hpp"
#include "message.hpp"
#include "utils.hpp"

namespace {

volatile std::sig_atomic_t g_stop = 0;

void onSigInt(int) {
    g_stop = 1;
}

bool recvString(void* socket, std::string* out) {
    zmq_msg_t msg;
    zmq_msg_init(&msg);

    int rc = zmq_msg_recv(&msg, socket, 0);
    if (rc == -1) {
        zmq_msg_close(&msg);
        return false;
    }

    const auto* data = static_cast<const char*>(zmq_msg_data(&msg));
    const auto size = static_cast<std::size_t>(zmq_msg_size(&msg));
    out->assign(data, size);

    zmq_msg_close(&msg);
    return true;
}

bool sendString(void* socket, const std::string& s, int flags = 0) {
    int rc = zmq_send(socket, s.data(), s.size(), flags);
    return rc != -1;
}

void setLingerZero(void* socket) {
    int linger = 0;
    zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger));
}

} // namespace

int main(int argc, char** argv) {
    const char* repEndpoint = "tcp://*:5555"; // commands
    const char* pubEndpoint = "tcp://*:5556"; // broadcast

    if (argc >= 2) {
        repEndpoint = argv[1];
    }
    if (argc >= 3) {
        pubEndpoint = argv[2];
    }

    std::signal(SIGINT, onSigInt);

    void* ctx = zmq_ctx_new();
    if (!ctx) {
        std::cerr << "Failed to create ZMQ context" << std::endl;
        return 1;
    }

    void* rep = zmq_socket(ctx, ZMQ_REP);
    void* pub = zmq_socket(ctx, ZMQ_PUB);
    if (!rep || !pub) {
        std::cerr << "Failed to create ZMQ sockets" << std::endl;
        if (rep) {
            zmq_close(rep);
        }
        if (pub) {
            zmq_close(pub);
        }
        zmq_ctx_term(ctx);
        return 1;
    }

    setLingerZero(rep);
    setLingerZero(pub);

    if (zmq_bind(rep, repEndpoint) != 0) {
        std::cerr << "REP bind failed: " << zmq_strerror(zmq_errno()) << std::endl;
        zmq_close(rep);
        zmq_close(pub);
        zmq_ctx_term(ctx);
        return 1;
    }

    if (zmq_bind(pub, pubEndpoint) != 0) {
        std::cerr << "PUB bind failed: " << zmq_strerror(zmq_errno()) << std::endl;
        zmq_close(rep);
        zmq_close(pub);
        zmq_ctx_term(ctx);
        return 1;
    }

    utils::logInfo(std::string("Server started. REP=") + repEndpoint + " PUB=" + pubEndpoint);

    GroupManager groups;

    while (!g_stop) {
        std::string req;
        if (!recvString(rep, &req)) {
            // most commonly interrupted by SIGINT
            if (zmq_errno() == EINTR && g_stop) {
                break;
            }
            continue;
        }

        auto msg = message::parseMessage(req);

        bool shouldPublish = false;
        std::string pubTopic;
        std::string pubBody;

        std::string reply;

        switch (msg.type) {
        case message::Type::CreateGroup: {
            if (msg.user.empty() || msg.group.empty()) {
                reply = message::makeReplyError("invalid_args");
                break;
            }

            if (!groups.createGroup(msg.group)) {
                reply = message::makeReplyError("group_exists_or_invalid");
                break;
            }

            // creator becomes a member
            groups.joinGroup(msg.group, msg.user);
            reply = message::makeReplyOk();
            break;
        }

        case message::Type::JoinGroup: {
            if (msg.user.empty() || msg.group.empty()) {
                reply = message::makeReplyError("invalid_args");
                break;
            }

            if (!groups.groupExists(msg.group)) {
                reply = message::makeReplyError("no_such_group");
                break;
            }

            groups.joinGroup(msg.group, msg.user);
            reply = message::makeReplyOk();
            break;
        }

        case message::Type::Send: {
            if (msg.user.empty() || msg.group.empty()) {
                reply = message::makeReplyError("invalid_args");
                break;
            }

            if (!groups.groupExists(msg.group)) {
                reply = message::makeReplyError("no_such_group");
                break;
            }

            if (!groups.isMember(msg.group, msg.user)) {
                reply = message::makeReplyError("not_a_member");
                break;
            }

            reply = message::makeReplyOk();
            shouldPublish = true;
            pubTopic = msg.group;
            pubBody = msg.user + ": " + msg.text;
            break;
        }

        default:
            reply = message::makeReplyError("unknown_command");
            break;
        }

        // REP: always reply
        sendString(rep, reply);

        // PUB: send multipart [topic][body]
        if (shouldPublish) {
            sendString(pub, pubTopic, ZMQ_SNDMORE);
            sendString(pub, pubBody, 0);
        }
    }

    utils::logInfo("Server shutting down...");

    zmq_close(rep);
    zmq_close(pub);
    zmq_ctx_term(ctx);

    return 0;
}
