#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include <iostream>

#include <zmq.h>

struct ReceiverContext {
    void* sub_socket = nullptr;
    std::atomic<bool> stop{false};

    std::mutex mtx;
    std::vector<std::string> pending_subscriptions;
};

inline void requestSubscribe(ReceiverContext& ctx, const std::string& topic) {
    std::lock_guard<std::mutex> lock(ctx.mtx);
    ctx.pending_subscriptions.push_back(topic);
}

inline void requestStop(ReceiverContext& ctx) {
    ctx.stop.store(true);
}

inline bool recvPart(void* socket, std::string* out, bool* hasMore) {
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

    int more = zmq_msg_more(&msg);
    *hasMore = (more != 0);

    zmq_msg_close(&msg);
    return true;
}

inline void applyPendingSubscriptions(ReceiverContext& ctx) {
    std::vector<std::string> subs;
    {
        std::lock_guard<std::mutex> lock(ctx.mtx);
        subs.swap(ctx.pending_subscriptions);
    }

    for (const auto& topic : subs) {
        if (topic.empty()) {
            continue;
        }
        zmq_setsockopt(ctx.sub_socket, ZMQ_SUBSCRIBE, topic.data(), topic.size());
    }
}

inline void* receiverThread(void* arg) {
    auto* ctx = static_cast<ReceiverContext*>(arg);
    if (!ctx || !ctx->sub_socket) {
        return nullptr;
    }

    while (!ctx->stop.load()) {
        applyPendingSubscriptions(*ctx);

        zmq_pollitem_t items[] = {
            {ctx->sub_socket, 0, ZMQ_POLLIN, 0},
        };

        // timeout in ms (so we can also react to stop/subscriptions)
        // Smaller value reduces the chance to miss messages right after join/subscribe.
        int rc = zmq_poll(items, 1, 50);
        if (rc == -1) {
            continue; // interrupted
        }

        if (items[0].revents & ZMQ_POLLIN) {
            std::string first;
            bool more = false;
            if (!recvPart(ctx->sub_socket, &first, &more)) {
                continue;
            }

            if (more) {
                std::string second;
                bool more2 = false;
                if (!recvPart(ctx->sub_socket, &second, &more2)) {
                    continue;
                }

                // Expected multipart: [topic][body]
                std::cout << "[" << first << "] " << second << std::endl;
            } else {
                // Fallback: single-frame message
                std::cout << first << std::endl;
            }

            std::cout.flush();
        }
    }

    // final apply, just in case
    applyPendingSubscriptions(*ctx);

    return nullptr;
}
