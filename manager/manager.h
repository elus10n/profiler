#ifndef MANAGER_H
#define MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>

#include "../metrics/metrics_collector.h"
#include "../processes/process_manager.h"

const uint64_t min_interval_ms = 300;

struct Configuration
{
    std::vector<MetricType> metrics = {MetricType::PAGE_FAULTS};
    uint64_t interval_ms = 500;

    Configuration() = default;
    Configuration(std::vector<MetricType>& metrics, uint64_t interval): metrics(std::move(metrics)), interval_ms(interval) {}

    bool is_valid() const
    {
        return !metrics.empty() && interval_ms >= min_interval_ms;
    }
};

const Configuration default_cfg;

using metric_callback = std::function<void(ProfilingSnapshot)>;
using error_callback = std::function<void(const std::string&)>;

class Manager
{
    std::unique_ptr<ProcessManager> manager;
    std::unique_ptr<MetricCollector> collector;

    metric_callback callback_metric;
    error_callback callback_error;

    std::atomic<bool> is_profiling_active{false};
    std::atomic<pid_t> current_pid{-1};
    std::string current_programm = "idle";
    Configuration current_config = default_cfg;

    public:
    Manager();
    ~Manager() = default;

    bool setup_and_start_profiling(const std::string& programm, const std::vector<std::string>& args, const Configuration& config);
    void stop_profiling();

    void setup_metrics_callback(metric_callback callback);
    void setup_error_callback(error_callback callback);

    private:
    bool is_active() const;
    pid_t get_current_pid() const;
    std::string get_current_programm() const;
    Configuration get_current_config() const;

    void report_error(const std::string& error);
    void report_metrics(const ProfilingSnapshot& snapshot);

    void on_metrics_recieved(const ProfilingSnapshot& snapshot);
    void on_error_recieved(const std::string& error);
};

#endif