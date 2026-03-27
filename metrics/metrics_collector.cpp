#include "metrics_collector.h"

#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <signal.h>
#include <cstring>
#include <system_error>

MetricCollector::MetricCollector()
{
    eventManager = std::make_unique<EventManager>();
    snapshotManager = std::make_unique<SnapshotManager>();
}

MetricCollector::~MetricCollector()
{
    stop_profiling();
}

bool MetricCollector::start_profiling(int pid, const std::vector<MetricType>& metrics, uint64_t interval_ms) 
{    
    if (!is_process_alive(pid))
    {
        report_error("[Profiler] Process " + std::to_string(pid) + " does not exist!");
        return false;
    }
    
    if (metrics.empty())
    {
        report_error("[Profiler] No metrics specified!");
        return false;
    }
    
    profiled_pid_ = pid;
    profiling_interval_ms_ = interval_ms;

    metrics_ = Converter::convert_types_to_metric(metrics);
    
    if (!eventManager->setup_perf_events(pid, metrics_)) 
    {
        return false;
    }
    
    profiling_active_ = true;
    
    profiling_thread_ = std::thread(&MetricCollector::profiling_loop, this);
    
    report_log("[Profiler] Started profiling PID " + std::to_string(pid) +  " with interval " + std::to_string(interval_ms) + "ms\n");
    return true;
}

void MetricCollector::stop_profiling() 
{
    if (profiling_active_.exchange(false))
    {
        if (profiling_thread_.joinable())
        {
            profiling_thread_.join();
        }

        eventManager->cleanup_perf_events();
        
        report_log("[Profiler] Stopped profiling PID " + std::to_string(profiled_pid_) + "\n");
    }
}

void MetricCollector::profiling_loop() 
{
    report_log("[Profiler] Profiling loop started for PID " + std::to_string(profiled_pid_) +"\n");
    
    while (profiling_active_ && is_process_alive(profiled_pid_)) 
    {
        auto interval_start = std::chrono::steady_clock::now();

        snapshotData data = eventManager->read_perf_events();
        
        ProfilingSnapshot snapshot = snapshotManager->collect_snapshot(profiling_interval_ms_, data);
        
        report_metrics(snapshot);
        
        auto elapsed = std::chrono::steady_clock::now() - interval_start;
        auto sleep_time = std::chrono::milliseconds(profiling_interval_ms_) - elapsed;
        if (sleep_time > std::chrono::milliseconds(0)) 
        {
            std::this_thread::sleep_for(sleep_time);
        }
    }
    if (!is_process_alive(profiled_pid_))
    {
        report_log("[Profiler] Profiled process " + std::to_string(profiled_pid_) + " has terminated\n");
    }

    report_log("[Profiler] Profiling loop finished\n");
}

bool MetricCollector::is_process_alive(int pid) 
{
    return (kill(pid, 0) == 0);
}

const std::vector<ProfilingSnapshot>& MetricCollector::get_snapshots() 
{
    return snapshotManager->getSnapshots();
}

void MetricCollector::setup_error_callback(ProfilingErrorCallback callback)
{
    error_callback_ = callback;
}

void MetricCollector::setup_metric_callback(ProfilingMetricCallback callback)
{
    metric_callback_ = callback;
}

void MetricCollector::setup_log_callback(ProfilingLogCallback callback)
{
    log_callback = callback;
}

void MetricCollector::report_error(const std::string& error)
{
    if(error_callback_)
    {
        error_callback_(error);
    }
    else
    {
        std::cerr << error;
    }
}

void MetricCollector::report_metrics(const ProfilingSnapshot& snapshot)
{
    if(metric_callback_)
    {
        metric_callback_(snapshot);
    }
    else
    {
        report_error("Undiefined metric_callback in MC!");
    }
}

void MetricCollector::report_log(const std::string& log)
{
    if(log_callback)
    {
        log_callback(log);
    }
    else
    {
        report_error("Undefined log_callback in MC!");
    }
}