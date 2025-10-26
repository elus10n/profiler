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
    if (profiling_active_)//нельзя запустить то, что уже идет
    {
        report_error("[Profiler] Profiling already active!");
        return false;
    }
    
    if (!is_process_alive(pid))//нельзя профилировать то, что умерло
    {
        report_error("[Profiler] Process " + std::to_string(pid) + " does not exist!");
        return false;
    }
    
    if (metrics.empty()) //профилирование по показателям: "никакие" - невозможно
    {
        report_error("[Profiler] No metrics specified!");
        return false;
    }
    
    profiled_pid_ = pid;
    profiling_interval_ms_ = interval_ms;
    
    if (!setup_perf_events(pid, metrics)) //не получилось установить perf events
    {
        report_error("[Profiler] Failed to setup perf events!");
        return false;
    }
    
    snapshots_.clear();//очищаем историю, если в ней вдруг уже что то было
    profiling_active_ = true;
    
    profiling_thread_ = std::thread(&MetricCollector::profiling_loop, this);//запускаем основную функцию в другом потоке
    
    std::cout << "[Profiler] Started profiling PID " << pid << " with interval " << interval_ms << "ms\n";
    return true;
}

void MetricCollector::stop_profiling() 
{
    if (profiling_active_.exchange(false)) //если профилировка была запущена -> вернет true и установит false
    {
        if (profiling_thread_.joinable()) //если активен -> вернет true
        {
            profiling_thread_.join();//блкируем текущий(основаной получается) поток до завершения потока сбора метрик.
        }

        cleanup_perf_events();//закрытие сбора метрик
        
        std::cout << "[Profiler] Stopped profiling PID " << profiled_pid_ << "\n";
        profiled_pid_ = -1;
    }
}

void MetricCollector::profiling_loop() 
{
    std::cout << "[Profiler] Profiling loop started for PID " << profiled_pid_ << "\n";
    
    while (profiling_active_ && is_process_alive(profiled_pid_)) //собираем метрики пока нужно собирать метрики и пока профилируемый процесс жив
    {
        auto interval_start = std::chrono::steady_clock::now();
        
        ProfilingSnapshot snapshot = collect_snapshot(profiling_interval_ms_);//чтение всех perf event'ов
        snapshots_.push_back(snapshot);//помещаем снапшот в историю
        
        if (metric_callback_)//если callback определен
        {
            metric_callback_(snapshot);//вызываем callback ..??????????????????????????????..
        }
        
        auto elapsed = std::chrono::steady_clock::now() - interval_start;
        auto sleep_time = std::chrono::milliseconds(profiling_interval_ms_) - elapsed;
        if (sleep_time > std::chrono::milliseconds(0)) 
        {
            std::this_thread::sleep_for(sleep_time);
        }//вся эта конструкция для того, чтобы один цикл сбора данных для снапшота занимал время, не меньшее, чем profiling_interval_ms_
    }
    
    if (!is_process_alive(profiled_pid_))//если профилируемый процесс завершен -> уведомляем об этом
    {
        std::cout << "[Profiler] Profiled process " << profiled_pid_ << " has terminated\n";
        
        if (metric_callback_)//если callback определен
        {
            ProfilingSnapshot final_snapshot = collect_snapshot(0);//финальный снапшот ..???????????????????????..
            metric_callback_(final_snapshot);//вызываем callback ..??????????????????????????????..
        }
    }
    
    std::cout << "[Profiler] Profiling loop finished\n";
}

ProfilingSnapshot MetricCollector::collect_snapshot(uint64_t duration_ms) 
{
    ProfilingSnapshot snapshot;

    snapshot.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();//абсолютное время собираемого снапшота
    snapshot.duration_ms = duration_ms;//установка интервала времени с прошлого снапшота
    
    for (auto& event : perf_events_)//обходим все perf_events
    {
        uint64_t current_value = read_perf_event(event.fd);//читаем c дескриптора ..?????????????????..
        
        uint64_t delta = current_value - event.last_value;//вычисляем дельту метрики
        event.last_value = current_value;//обновляем абсолютное значение
        
        MetricValue metric;//создаем метрику
        metric.type = event.type;//тип берем из perf_event'а
        metric.value = delta;//устанавливаем значение
        
        switch (event.type)//исходя из типа perf_event'а устанавливаем оставшиеся поля метрики
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

        snapshot.metrics.push_back(metric);//метрику пушим в снапшот
    }
    return snapshot;
}

bool MetricCollector::setup_perf_events(int pid, const std::vector<MetricType>& metrics) 
{
    for (auto metric_type : metrics)//обходим все типы наблюдаемых метрик
    {
        int fd = open_perf_event(pid, metric_type);//создаем дескриптор с открытым сбором метрики для pid'а
        if (fd < 0)//если дескриптор открылся с ошибкой
        {
            report_error("[Profiler] Failed to open perf event for metric type " + std::to_string(static_cast<int>(metric_type)));
            cleanup_perf_events();//сворачиваем все perf_event'ы
            return false;
        }
        
        perf_events_.push_back({fd, metric_type, 0});//если успешно, то текущий perf_event отправляется в сущетсвующие perf_event'ы
        std::cout << "[Profiler] Opened perf event for " << perf_events_.back().fd << "\n";
    }
    
    return true;
}

void MetricCollector::cleanup_perf_events() 
{
    for (auto& event : perf_events_)//для каждого perf_event'а из текущих
    {
        if (event.fd >= 0)//если дескриптор нормальный
        {
            close(event.fd);//закрываем
        }
    }
    perf_events_.clear();//в конце очищаем вектор
}

int MetricCollector::open_perf_event(int pid, MetricType type) 
{
    struct perf_event_attr attr;//системная структура
    memset(&attr, 0, sizeof(attr));//обнуляем структуру
    attr.size = sizeof(attr);
    attr.type = PERF_TYPE_HARDWARE; // Используем аппаратные счетчики процессора
    attr.disabled = 1;              // Начинаем в выключенном состоянии
    attr.exclude_kernel = 1;        // Исключаем события в kernel space
    attr.exclude_hv = 1;            // Исключаем события в гипервизоре
    
    switch (type)//исходя из типа метрики, выбираем конфигурацию сбора (по надобности меняем тип сбора на программный)
    {
        case MetricType::INSTRUCTIONS:
            attr.config = PERF_COUNT_HW_INSTRUCTIONS;
            break;
        case MetricType::CPU_CYCLES:
            attr.config = PERF_COUNT_HW_CPU_CYCLES;
            break;
        case MetricType::CACHE_MISSES:
            attr.config = PERF_COUNT_HW_CACHE_MISSES;
            break;
        case MetricType::CACHE_REFERENCES:
            attr.config = PERF_COUNT_HW_CACHE_REFERENCES;
            break;
        case MetricType::BRANCH_MISSES:
            attr.config = PERF_COUNT_HW_BRANCH_MISSES;
            break;
        case MetricType::PAGE_FAULTS:
            attr.type = PERF_TYPE_SOFTWARE; // Страничные нарушения - software event
            attr.config = PERF_COUNT_SW_PAGE_FAULTS;
            break;
        case MetricType::CONTEXT_SWITCHES:
            attr.type = PERF_TYPE_SOFTWARE; // Переключения контекста - software event  
            attr.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
            break;
        default:
            return -1;
    }
    
    int fd = perf_event_open(&attr, pid, -1, -1, 0);//делаем системный вызов через обертку с уже сформированной структурой 
    if (fd < 0)//если дескриптор открылся с ошибкой - ретерн
    {
        return -1;
    }
    
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);  // Сброс в 0
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); // Включение счетчика
    
    return fd;
}

uint64_t MetricCollector::read_perf_event(int fd) 
{
    uint64_t value = 0;//значение для читаемой метрики
    if (read(fd, &value, sizeof(value)) != sizeof(value))//если чтение неуспешно
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
    return snapshots_;//возврат всех снапшотов (вся история)
}

void MetricCollector::setup_error_callback(ProfilingErrorCallback callback)
{
    error_callback_ = callback;
}

void MetricCollector::setup_metric_callback(ProfilingMetricCallback callback)
{
    metric_callback_ = callback;
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
        report_error("Undiefined metric_callback_in_MC!");
    }
}