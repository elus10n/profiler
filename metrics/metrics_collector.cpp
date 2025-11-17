#include "metrics_collector.h"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <signal.h>
#include <cstring>
#include <system_error>

static long perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) 
{
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

MetricCollector::MetricCollector() = default;

MetricCollector::~MetricCollector()
{
    stop_profiling();
}


bool MetricCollector::start_profiling(int pid, const std::vector<MetricType>& metrics, uint64_t interval_ms) 
{
    if (profiling_active_)
    {
        report_error("[Profiler] Profiling already active!");
        return false;
    }
    
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
    
    if (!setup_perf_events(pid, metrics)) 
    {
        return false;
    }
    
    snapshots_.clear();
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

        cleanup_perf_events();
        
        report_log("[Profiler] Stopped profiling PID " + std::to_string(profiled_pid_) + "\n");
        profiled_pid_ = -1;
    }
}

void MetricCollector::profiling_loop() 
{
    report_log("[Profiler] Profiling loop started for PID " + std::to_string(profiled_pid_) +"\n");
    
    while (profiling_active_ && is_process_alive(profiled_pid_)) 
    {
        auto interval_start = std::chrono::steady_clock::now();
        
        ProfilingSnapshot snapshot = collect_snapshot(profiling_interval_ms_);
        snapshots_.push_back(snapshot);
        
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
        
        if (metric_callback_)
        {
            ProfilingSnapshot final_snapshot = collect_snapshot(0);
            metric_callback_(final_snapshot);
        }
    }

    report_log("[Profiler] Profiling loop finished\n");
}

ProfilingSnapshot MetricCollector::collect_snapshot(uint64_t duration_ms) 
{
    ProfilingSnapshot snapshot;

    snapshot.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();//абсолютное время собираемого снапшота
    snapshot.duration_ms = duration_ms;
    
    for (auto& event : perf_events_)
    {
        uint64_t current_value = read_perf_event(event.fd);
        
        uint64_t delta = current_value - event.last_value;
        event.last_value = current_value;
        
        MetricValue metric;
        metric.type = event.type;
        metric.value = delta;
        
        switch (event.type)
        {
            case MetricType::INSTRUCTIONS:
                metric.name = "instructions";
                metric.unit = "count";
                break;
            case MetricType::CPU_CYCLES:
                metric.name = "cpu_cycles";
                metric.unit = "cycles";
                break;
            case MetricType::CACHE_MISSES:
                metric.name = "cache_misses";
                metric.unit = "misses";
                break;
            case MetricType::CACHE_REFERENCES:
                metric.name = "cache_references";
                metric.unit = "references";
                break;
            case MetricType::BRANCH_MISSES:
                metric.name = "branch_misses";
                metric.unit = "misses";
                break;
            case MetricType::PAGE_FAULTS:
                metric.name = "page_faults";
                metric.unit = "faults";
                break;
            case MetricType::CONTEXT_SWITCHES:
                metric.name = "context_switches";
                metric.unit = "switches";
                break;
        }
        snapshot.metrics.push_back(metric);
    }
    return snapshot;
}

bool MetricCollector::setup_perf_events(int pid, const std::vector<MetricType>& metrics) 
{
    for (auto metric_type : metrics)
    {
        int fd = open_perf_event(pid, metric_type);
        if (fd < 0)
        {
            cleanup_perf_events();
            return false;
        }
        
        perf_events_.push_back({fd, metric_type, 0});
    }
    
    return true;
}

void MetricCollector::cleanup_perf_events() 
{
    for (auto& event : perf_events_)
    {
        if (event.fd >= 0)
        {
            close(event.fd);
        }
    }
    perf_events_.clear();
}

int MetricCollector::open_perf_event(int pid, MetricType type) 
{
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.disabled = 1;              
    attr.exclude_kernel = 0;        
    attr.exclude_hv = 1;            
    
    switch (type)
    {
        case MetricType::INSTRUCTIONS:
            attr.type = PERF_TYPE_HARDWARE; 
            attr.config = PERF_COUNT_HW_INSTRUCTIONS;
            break;
        case MetricType::CPU_CYCLES:
            attr.type = PERF_TYPE_HARDWARE; 
            attr.config = PERF_COUNT_HW_CPU_CYCLES;
            break;
        case MetricType::CACHE_MISSES:
            attr.type = PERF_TYPE_HARDWARE; 
            attr.config = PERF_COUNT_HW_CACHE_MISSES;
            break;
        case MetricType::CACHE_REFERENCES:
            attr.type = PERF_TYPE_HARDWARE; 
            attr.config = PERF_COUNT_HW_CACHE_REFERENCES;
            break;
        case MetricType::BRANCH_MISSES:
            attr.type = PERF_TYPE_HARDWARE; 
            attr.config = PERF_COUNT_HW_BRANCH_MISSES;
            break;
        case MetricType::PAGE_FAULTS:
            attr.type = PERF_TYPE_SOFTWARE; 
            attr.config = PERF_COUNT_SW_PAGE_FAULTS;
            break;
        case MetricType::CONTEXT_SWITCHES:
            attr.type = PERF_TYPE_SOFTWARE;  
            attr.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
            break;
        default:
            return -1;
    }
    
    int fd = perf_event_open(&attr, pid, -1, -1, 0);
    if (fd < 0)
    {
        report_error("[Profiler]Failed to open perf event " + std::to_string(static_cast<int>(type)) + " pid: " + std::to_string(pid));
        return -1;
    }
    
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    
    return fd;
}

uint64_t MetricCollector::read_perf_event(int fd) 
{
    uint64_t value = 0;
    if (read(fd, &value, sizeof(value)) != sizeof(value))
    {
        return 0;
    }
    return value;
}

bool MetricCollector::is_process_alive(int pid) 
{
    return (kill(pid, 0) == 0);
}

const std::vector<ProfilingSnapshot>& MetricCollector::get_snapshots() const 
{
    return snapshots_;
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

std::ostream& operator<<(std::ostream& ostr, const ProfilingSnapshot& snapshot)
{
    for(const auto& metric : snapshot.metrics)
    {
        ostr << metric.name << ": " << metric.value << std::endl;
    }
    return ostr;
}