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

class MetricCollector 
{
public:
    MetricCollector();
    ~MetricCollector();

    bool start_profiling(int pid, const std::vector<MetricType>& metrics, uint64_t interval_ms = 100);
    
    void stop_profiling();
    
    const std::vector<ProfilingSnapshot>& get_snapshots();

    void setup_error_callback(ProfilingErrorCallback callback);
    void setup_metric_callback(ProfilingMetricCallback callback);
    void setup_log_callback(ProfilingLogCallback callback);
    
    bool is_profiling() const { return profiling_active_; }

private:
    std::atomic<bool> profiling_active_{false};  
    std::atomic<int> profiled_pid_{-1};
    
    std::unique_ptr<EventManager> eventManager;
    std::unique_ptr<SnapshotManager> snapshotManager;

    std::thread profiling_thread_;                    

    ProfilingMetricCallback metric_callback_; 
    ProfilingErrorCallback error_callback_;   
    ProfilingLogCallback log_callback;

    uint64_t profiling_interval_ms_;   
    
    std::vector<std::shared_ptr<Metric>> metrics_;
    
    void profiling_loop();                                    

    void report_error(const std::string& error);
    void report_metrics(const ProfilingSnapshot& snapshot);
    void report_log(const std::string& log);

    bool is_process_alive(int pid);
};

#endif