#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H 

#include "EventCreator.h"
#include "SnapshotManager.h"
#include "Converter.h"

#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>
#include <iostream>
#include <memory>

using ProfilingMetricCallback = std::function<void(const ProfilingSnapshot& snapshot)>;
using ProfilingErrorCallback = std::function<void(const std::string& error)>;
using ProfilingLogCallback = std::function<void(const std::string& log)>;

const int freq_cpu_hotspot = 5000;
const int cache_miss_bound = 10;
const int branch_miss_bound = 5;

enum class Modes
{
    COUNTING, CPU_HOTSPOT, C_M_HOTSPOT, B_HOTSPOT
};

const uint32_t min_interval_ms = 100;
const uint32_t max_interval_ms = 5000;

struct ProfilingConfiguration
{
    Modes mode;
    std::vector<MetricType> metrics;
    int interval_ms;

    ProfilingConfiguration() = default;
    ProfilingConfiguration(const Modes mode, std::vector<MetricType>& metrics, const int interval) : mode(mode), metrics(std::move(metrics)), interval_ms(interval) {}
};

class MetricCollector 
{
public:
    MetricCollector();
    ~MetricCollector();

    bool start_profiling(int pid, const ProfilingConfiguration& config);
    
    void stop_profiling();
    
    const std::vector<ProfilingSnapshot>& get_snapshots();

    void setup_error_callback(ProfilingErrorCallback callback);
    void setup_metric_callback(ProfilingMetricCallback callback);
    void setup_log_callback(ProfilingLogCallback callback);
    void get_hotspot_data();
    
    bool is_profiling() const { return profiling_active_; }

private:
    Modes mode;

    std::atomic<bool> profiling_active_{false};  
    std::atomic<int> profiled_pid_{-1};
    
    std::unique_ptr<EventManager> eventManager;
    std::unique_ptr<SnapshotManager> snapshotManager;

    std::thread profiling_thread_;                    

    ProfilingMetricCallback metric_callback_; 
    ProfilingErrorCallback error_callback_;   
    ProfilingLogCallback log_callback;

    uint64_t profiling_interval_ms_;   
    
    void profiling_loop();                                    

    void report_error(const std::string& error);
    void report_metrics(const ProfilingSnapshot& snapshot);
    void report_log(const std::string& log);

    bool is_process_alive(int pid);
};

#endif