#ifndef EVENTCREATOR_H
#define EVENTCREATOR_H

#include "metrics.h"

#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <signal.h>
#include <cstring>
#include <system_error>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <sys/mman.h>

struct PerfEvent 
{
    int fd; 
    std::shared_ptr<Metric> metric; 
    void* mmap_memory = nullptr;   
    long len = 0;                  
};

struct EventConfiguration
{
    std::vector<std::shared_ptr<Metric>> metrics;
    bool is_sampling;
    bool is_freq;
    int count;

    EventConfiguration() = default;
    EventConfiguration(std::vector<std::shared_ptr<Metric>>& metrics, const bool is_sampling, bool is_freq , const int count) : metrics(std::move(metrics)), is_sampling(is_sampling), is_freq(is_freq), count(count) {}
};

class EventManager
{               
    std::vector<PerfEvent> perf_events_; 

    static long perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) 
    {
        return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
    }

    uint64_t read_perf_event(PerfEvent& event)
    {
        uint64_t value = 0;
        if (read(event.fd, &value, sizeof(value)) != sizeof(value)) return 0;

        return value;
    }

    public:

    bool setup_perf_events(int pid, const EventConfiguration& config) 
    {
        if(config.is_sampling)
        {
            int fd = open_perf_event(pid, config.metrics[0], config.is_sampling, config.is_freq, config.count);
            if (fd < 0)
            {
                cleanup_perf_events(); // надо добавить поддержку закрытия hotspot event'а
                //надо проверить, нужно ли закрывать неоткрывшийся дескриптор (тут то он один)
                return false;
            }
            long page_size = sysconf(_SC_PAGESIZE);
            long mmap_size = (1 + 16) * page_size;
            void* data = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if(data == MAP_FAILED)
            {
                cleanup_perf_events();
                return false;
            }
            perf_events_.push_back({fd, config.metrics[0], data, mmap_size});
        }
        else
        {
            for (const auto& metric : config.metrics)
            {
                int fd = open_perf_event(pid, metric);
                if (fd < 0)
                {
                    cleanup_perf_events();
                    return false;
                }
                
                perf_events_.push_back({fd, metric});
            }
        }

        return true;
    }

    void cleanup_perf_events() 
    {
        for (auto& event : perf_events_)
        {
            if(void* data = event.mmap_memory) munmap(data, event.len);
            if(event.fd >= 0) close(event.fd);
        }
        perf_events_.clear();
    }

    int open_perf_event(int pid, const std::shared_ptr<Metric>& type, bool is_sampling = false, bool is_freq = false, int count = 0) 
    {
        struct perf_event_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.size = sizeof(attr);
        attr.disabled = 1;              
        attr.exclude_kernel = 0;        
        attr.exclude_hv = 1;
        attr.type = type->type_;
        attr.config = type->config_;  
        if(is_sampling)
        {
            attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_CALLCHAIN; 
            if(is_freq)
            {
                attr.sample_freq = count; 
                attr.freq = 1;
            }     
            else
            {
                attr.sample_period = count; 
                attr.freq = 0;
            }
        }
        
        int fd = perf_event_open(&attr, pid, -1, -1, 0);
        if (fd < 0)  return -1;
        //добавить обработку ошибок
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        
        return fd;
    }

    std::map<std::shared_ptr<Metric>, uint64_t> read_counting_raw()
    {
        std::map<std::shared_ptr<Metric>, uint64_t> perf_events;

        for(auto& event : perf_events_)
            perf_events[event.metric] = read_perf_event(event);

        return perf_events;
    }

    HotspotRawData read_hotspot_raw()
    {
        if (perf_events_.empty() || perf_events_[0].mmap_memory == nullptr) 
            return {0, 0, nullptr, 0};

        auto& ev = perf_events_[0];
        auto* meta = static_cast<struct perf_event_mmap_page*>(ev.mmap_memory);

        HotspotRawData raw;
        
        raw.head = meta->data_head;
        __sync_synchronize();
        
        raw.tail = meta->data_tail;
        
        long page_size = sysconf(_SC_PAGESIZE);
        
        raw.data_start = static_cast<uint8_t*>(ev.mmap_memory) + page_size;
        
        raw.data_mask = (ev.len - page_size) - 1;

        return raw;
    }

    void update_tail(uint64_t new_tail) 
    {
        if (!perf_events_.empty() && perf_events_[0].mmap_memory) 
        {
            auto* meta = static_cast<struct perf_event_mmap_page*>(perf_events_[0].mmap_memory);
            meta->data_tail = new_tail;
        }
    }
};

#endif