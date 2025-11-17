#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H 

#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>
#include <iostream>

enum class MetricType 
{
    INSTRUCTIONS,      
    CPU_CYCLES,        
    CACHE_MISSES,      
    CACHE_REFERENCES,  
    BRANCH_MISSES,     
    PAGE_FAULTS,      
    CONTEXT_SWITCHES   
};


struct MetricValue 
{
    MetricType type;    
    uint64_t value;     
    std::string name;   
    std::string unit;   
};


struct ProfilingSnapshot 
{
    std::vector<MetricValue> metrics;
    uint64_t timestamp_ms;             
    uint64_t duration_ms;              
    
    const MetricValue* find_metric(MetricType type) const 
    {
        for (const auto& metric : metrics) 
        {
            if (metric.type == type) 
            {
                return &metric;
            }
        }
        return nullptr;
    }

    friend std::ostream& operator<<(std::ostream& ostr, const ProfilingSnapshot& snapshot);
};

using ProfilingMetricCallback = std::function<void(const ProfilingSnapshot& snapshot)>;
using ProfilingErrorCallback = std::function<void(const std::string& error)>;
using ProfilingLogCallback = std::function<void(const std::string& log)>;

class MetricCollector 
{
public:
    MetricCollector();
    ~MetricCollector();
    
    MetricCollector(const MetricCollector&) = delete;
    MetricCollector& operator=(const MetricCollector&) = delete;

    bool start_profiling(int pid, const std::vector<MetricType>& metrics, uint64_t interval_ms = 100);
    
    void stop_profiling();
    
    const std::vector<ProfilingSnapshot>& get_snapshots() const;

    void setup_error_callback(ProfilingErrorCallback callback);
    void setup_metric_callback(ProfilingMetricCallback callback);
    void setup_log_callback(ProfilingLogCallback callback);
    
    bool is_profiling() const { return profiling_active_; }
    int get_profiled_pid() const { return profiled_pid_; }

private:
    struct PerfEvent 
    {
        int fd;                
        MetricType type;       
        uint64_t last_value;    
    };
    
    std::atomic<bool> profiling_active_{false};  
    std::atomic<int> profiled_pid_{-1};           

    std::thread profiling_thread_;              
    std::vector<PerfEvent> perf_events_;        

    ProfilingMetricCallback metric_callback_; 
    ProfilingErrorCallback error_callback_;   
    ProfilingLogCallback log_callback;

    std::vector<ProfilingSnapshot> snapshots_; 
    uint64_t profiling_interval_ms_;            
    
    void profiling_loop();                     
    bool setup_perf_events(int pid, const std::vector<MetricType>& metrics);
    void cleanup_perf_events();                 
    ProfilingSnapshot collect_snapshot(uint64_t duration_ms);

    void report_error(const std::string& error);
    void report_metrics(const ProfilingSnapshot& snapshot);
    void report_log(const std::string& log);
    
    int open_perf_event(int pid, MetricType type);
    uint64_t read_perf_event(int fd);
    bool is_process_alive(int pid);
};

#endif