#ifndef MANAGER_H
#define MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>

#include "../metrics/metrics_collector.h"
#include "../processes/process_manager.h"

const uint32_t min_interval_ms = 100;
const uint32_t max_interval_ms = 5000;

struct ProfilingConfiguration
{
    std::vector<MetricType> metrics = {MetricType::PAGE_FAULTS};
    int interval_ms = 500;

    ProfilingConfiguration() = default;
    ProfilingConfiguration(std::vector<MetricType>& metrics, int interval): metrics(std::move(metrics)), interval_ms(interval) {}

    bool is_valid() const
    {
        return !metrics.empty() && interval_ms >= min_interval_ms && interval_ms <= max_interval_ms;
    }

    friend std::ostream& operator<<(std::ostream& ostr, const ProfilingConfiguration pr_config);
};

const ProfilingConfiguration default_cfg;

using metric_callback = std::function<void(ProfilingSnapshot)>;
using error_callback = std::function<void(const std::string&)>;
using log_callback = std::function<void(const std::string& log)>;

class Manager
{
    std::unique_ptr<MetricCollector> collector;
    std::unique_ptr<ProcessManager> manager;

    metric_callback callback_metric;
    error_callback callback_error;
    log_callback callback_log;

    std::atomic<pid_t> current_pid{-1};
    std::string current_programm = "idle";
    ProfilingConfiguration current_config = default_cfg;

    public:
    Manager();
    ~Manager() = default;

    void setup();
    bool start_profiling(const std::string& programm, const std::vector<std::string>& args, const ProfilingConfiguration& config);
    void stop_profiling();

    void setup_metrics_callback(metric_callback callback);
    void setup_error_callback(error_callback callback);
    void setup_log_callback(log_callback callback);
    bool is_active() const;
    bool is_process_alive() const;

    pid_t get_current_pid() const;
    std::string get_current_programm() const;
    ProfilingConfiguration get_current_config() const;

    private:

    void report_error(const std::string& error);
    void report_metrics(const ProfilingSnapshot& snapshot);
    void report_log(const std::string& log);

    void on_metrics_recieved(const ProfilingSnapshot& snapshot);
    void on_error_recieved(const std::string& error);
    void on_log_recieved(const std::string& log);
};

#endif