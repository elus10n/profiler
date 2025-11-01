#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H 

#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>

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
    MetricType type;    // Тип метрики 
    uint64_t value;     // Числовое значение (дельта за последний интервал)
    std::string name;   // Человеко-читаемое имя 
    std::string unit;   // Единицы измерения 
};


struct ProfilingSnapshot 
{
    std::vector<MetricValue> metrics;
    uint64_t timestamp_ms;             // Абсолютное время сбора 
    uint64_t duration_ms;              // Длительность интервала измерения 
    
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
};

using ProfilingMetricCallback = std::function<void(const ProfilingSnapshot& snapshot)>;
using ProfilingErrorCallback = std::function<void(const std::string& error)>;

class MetricCollector 
{
public:
    MetricCollector();
    ~MetricCollector();
    
    MetricCollector(const MetricCollector&) = delete;
    MetricCollector& operator=(const MetricCollector&) = delete;

    bool start_profiling(int pid, const std::vector<MetricType>& metrics, uint64_t interval_ms = 100);
    
    void stop_profiling();
    
    const std::vector<ProfilingSnapshot>& get_snapshots() const;//возврат всех снимков

    void setup_error_callback(ProfilingErrorCallback callback);
    void setup_metric_callback(ProfilingMetricCallback callback);
    
    bool is_profiling() const { return profiling_active_; }
    int get_profiled_pid() const { return profiled_pid_; }

private:
    struct PerfEvent 
    {
        int fd;                 // Файловый дескриптор Linux perf event
        MetricType type;        // Тип метрики который он измеряет
        uint64_t last_value;    // Последнее значение для delta вычислений
    };
    
    std::atomic<bool> profiling_active_{false};  // Флаг активности 
    std::atomic<int> profiled_pid_{-1};          // Текущий PID 
    std::atomic<bool> should_stop{false};        // для того, чтобы profiling loop своевременно останавливался

    std::thread profiling_thread_;               // Поток в котором работает сбор метрик
    std::vector<PerfEvent> perf_events_;         // Все открытые perf events
    ProfilingMetricCallback metric_callback_;  // Callback для уведомлений
    ProfilingErrorCallback error_callback_;    // Callback для ошибок
    std::vector<ProfilingSnapshot> snapshots_;   // История всех собранных снимков
    uint64_t profiling_interval_ms_;             // Интервал между измерениями
    
    void profiling_loop();                       // Главный цикл в отдельном потоке
    bool setup_perf_events(int pid, const std::vector<MetricType>& metrics);
    void cleanup_perf_events();                  // Освобождение системных ресурсов
    ProfilingSnapshot collect_snapshot(uint64_t duration_ms);

    void report_error(const std::string& error);
    void report_metrics(const ProfilingSnapshot& snapshot);
    
    int open_perf_event(int pid, MetricType type);
    uint64_t read_perf_event(int fd);
    bool is_process_alive(int pid);
};

#endif