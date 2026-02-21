#ifndef DRPC_PROFILING_H
#define DRPC_PROFILING_H

#include <drpc/common.h>

namespace drpc {

struct ProfilingData {
    uint64_t total_requests = 0;
    uint64_t total_bytes = 0;
    uint64_t total_errors = 0;
    
    double avg_latency_us = 0.0;
    double min_latency_us = 0.0;
    double max_latency_us = 0.0;
    double p50_latency_us = 0.0;
    double p95_latency_us = 0.0;
    double p99_latency_us = 0.0;
    
    double throughput_mbps = 0.0;
    double throughput_ops = 0.0;
    
    double duration_sec = 0.0;
};

class Profiler {
public:
    Profiler() = default;
    ~Profiler() = default;

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    void recordLatency(uint64_t latency_ns);
    void recordBytes(size_t bytes);
    void recordError();

    ProfilingData getStats() const;
    void reset();

    static std::string format(const ProfilingData& data);

    void setMaxSamples(size_t max) { max_samples_ = max; }

private:
    void updatePercentiles(ProfilingData& data) const;

    std::vector<uint64_t> latencies_;
    std::atomic<uint64_t> total_bytes_{0};
    std::atomic<uint64_t> total_errors_{0};
    mutable std::mutex mutex_;
    
    std::chrono::steady_clock::time_point start_time_;
    size_t max_samples_ = 1000000;
};

class ScopedTimer {
public:
    explicit ScopedTimer(Profiler& profiler);
    ~ScopedTimer();

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    void stop();
    uint64_t elapsedNs() const;

private:
    Profiler& profiler_;
    std::chrono::steady_clock::time_point start_;
    bool stopped_ = false;
};

}
#endif
