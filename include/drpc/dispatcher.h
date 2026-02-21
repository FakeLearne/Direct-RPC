#ifndef DRPC_DISPATCHER_H
#define DRPC_DISPATCHER_H

#include <drpc/common.h>
#include <drpc/transport.h>
#include <drpc/buffer.h>
#include <drpc/protocol.h>
#include <thread>
#include <infiniband/verbs.h>

namespace drpc {

using WCCallback = std::function<void(const ibv_wc& wc)>;

class Dispatcher {
public:
    explicit Dispatcher(CompletionQueue& cq);
    ~Dispatcher();

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    void start();
    void stop();

    bool running() const { return running_.load(); }

    void registerCallback(uint64_t wr_id, WCCallback cb);
    void unregisterCallback(uint64_t wr_id);
    bool hasCallback(uint64_t wr_id) const;

    void pollOnce();

private:
    void pollLoop();

    CompletionQueue& cq_;
    std::thread poll_thread_;
    std::atomic<bool> running_{false};

    std::unordered_map<uint64_t, WCCallback> callbacks_;
    mutable std::mutex cb_mutex_;
};

struct RequestContext {
    uint64_t id = 0;
    uint32_t seq_id = 0;
    uint32_t func_id = 0;
    Buffer send_buf;
    Buffer recv_buf;
    std::function<void(Buffer& response)> user_callback;
    std::chrono::steady_clock::time_point start_time;
    bool completed = false;
};

class RequestContextManager {
public:
    RequestContextManager() = default;
    ~RequestContextManager() = default;

    RequestContextManager(const RequestContextManager&) = delete;
    RequestContextManager& operator=(const RequestContextManager&) = delete;

    uint64_t createContext(uint32_t seq_id, uint32_t func_id,
                           Buffer& send_buf, Buffer& recv_buf,
                           std::function<void(Buffer&)> callback);

    RequestContext* getContext(uint64_t id);
    void removeContext(uint64_t id);
    bool hasContext(uint64_t id) const;

    size_t pendingCount() const;
    void setTimeout(uint64_t id, int timeout_ms);
    std::vector<uint64_t> getTimedOutRequests();

private:
    std::unordered_map<uint64_t, RequestContext> contexts_;
    mutable std::mutex mutex_;
    std::atomic<uint64_t> next_id_{1};
};

}
#endif
