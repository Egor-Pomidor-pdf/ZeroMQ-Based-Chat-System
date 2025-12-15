#include <cstring>
#include <iostream>
#include <string>

#include <pthread.h>
#include <zmq.h>

#include "message.hpp"
#include "receiver_thread.hpp"
#include "ui.hpp"

namespace {

bool sendString(void* socket, const std::string& s) {
    int rc = zmq_send(socket, s.data(), s.size(), 0);
    return rc != -1;
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

} // namespace

int main(int argc, char** argv) {
    const char* reqEndpoint = "tcp://127.0.0.1:5555";
    const char* subEndpoint = "tcp://127.0.0.1:5556";
    
    std::string user;
    std::cout << "Enter your name: " << std::flush;
    std::getline(std::cin, user);
    if (user.empty()) {
        user = "anon";
    }

    void* ctx = zmq_ctx_new();
    if (!ctx) {
        std::cerr << "Failed to create ZMQ context" << std::endl;
        return 1;
    }

    void* req = zmq_socket(ctx, ZMQ_REQ);
    void* sub = zmq_socket(ctx, ZMQ_SUB);

    if (!req || !sub) {
        std::cerr << "Failed to create ZMQ sockets" << std::endl;
        if (req) {
            zmq_close(req);
        }
        if (sub) {
            zmq_close(sub);
        }
        zmq_ctx_term(ctx);
        return 1;
    }


    if (zmq_connect(req, reqEndpoint) != 0) {
        std::cerr << "REQ connect failed: " << zmq_strerror(zmq_errno()) << std::endl;
        zmq_close(req);
        zmq_close(sub);
        zmq_ctx_term(ctx);
        return 1;
    }

    if (zmq_connect(sub, subEndpoint) != 0) {
        std::cerr << "SUB connect failed: " << zmq_strerror(zmq_errno()) << std::endl;
        zmq_close(req);
        zmq_close(sub);
        zmq_ctx_term(ctx);
        return 1;
    }

    // Receiver thread owns SUB socket operations (subscribe is applied inside receiver thread).
    ReceiverContext rx;
    rx.sub_socket = sub;

    pthread_t thread{};
    if (pthread_create(&thread, nullptr, receiverThread, &rx) != 0) {
        std::cerr << "Failed to start receiver thread" << std::endl;
        zmq_close(req);
        zmq_close(sub);
        zmq_ctx_term(ctx);
        return 1;
    }

    ui::printHelp();

    while (true) {
        ui::printPrompt();

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        auto cmd = ui::parseCommandLine(line);

        if (cmd.type == ui::CommandType::Help) {
            ui::printHelp();
            continue;
        }

        if (cmd.type == ui::CommandType::Exit) {
            break;
        }

        if (cmd.type == ui::CommandType::Unknown) {
            std::cout << "Unknown command. Type 'help'." << std::endl;
            continue;
        }

        message::Message m;
        if (cmd.type == ui::CommandType::CreateGroup) {
            m.type = "CREATE_GROUP";
            m.user = user;
            m.group = cmd.group;
            m.text = "";
        } else if (cmd.type == ui::CommandType::JoinGroup) {
            m.type = "JOIN_GROUP";
            m.user = user;
            m.group = cmd.group;
            m.text = "";
        } else if (cmd.type == ui::CommandType::Send) {
            m.type = "SEND";
            m.user = user;
            m.group = cmd.group;
            m.text = cmd.text;
        }

        const std::string payload = message::toString(m);
        if (!sendString(req, payload)) {
            std::cerr << "Failed to send request" << std::endl;
            continue;
        }

        std::string reply;
        if (!recvString(req, &reply)) {
            std::cerr << "Failed to receive reply" << std::endl;
            continue;
        }

        std::cout << reply << std::endl;

        // After successful CREATE_GROUP / JOIN_GROUP, subscribe to group.
        // Note: With PUB-SUB, messages before the subscription reaches the server can be dropped ("slow joiner").
        if ((cmd.type == ui::CommandType::CreateGroup || cmd.type == ui::CommandType::JoinGroup) &&
            reply.rfind("OK", 0) == 0) {
            requestSubscribe(rx, cmd.group);
            std::cout << "Subscribed to '" << cmd.group << "'" << std::endl;
        }
    }

    requestStop(rx);
    pthread_join(thread, nullptr);

    zmq_close(req);
    zmq_close(sub);
    zmq_ctx_term(ctx);

    return 0;
}
