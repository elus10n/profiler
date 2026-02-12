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

struct PerfEvent 
{
    int fd; 
    std::shared_ptr<Metric> metric;                      
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
        if (read(event.fd, &value, sizeof(value)) != sizeof(value))
        {
            return 0;
        }
        return value;
    }

    public:

    bool setup_perf_events(int pid, const std::vector<std::shared_ptr<Metric>>& metrics) 
    {
        for (const auto& metric : metrics)
        {
            int fd = open_perf_event(pid, metric);
            if (fd < 0)
            {
                cleanup_perf_events();
                return false;
            }
            
            perf_events_.push_back({fd, metric});
        }
        
        return true;
    }

    void cleanup_perf_events() 
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

    int open_perf_event(int pid, const std::shared_ptr<Metric>& type) 
    {
        struct perf_event_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.size = sizeof(attr);
        attr.disabled = 1;              
        attr.exclude_kernel = 0;        
        attr.exclude_hv = 1;
        attr.type = type->type_;
        attr.config = type->config_;            
        
        int fd = perf_event_open(&attr, pid, -1, -1, 0);
        if (fd < 0)
        {
            return -1;
        }
        
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        
        return fd;
    }

    std::map<std::shared_ptr<Metric>, uint64_t> read_perf_events()
    {
        std::map<std::shared_ptr<Metric>, uint64_t> perf_events;

        for(auto& event : perf_events_)
        {
            perf_events[event.metric] = read_perf_event(event);
        }

        return perf_events;
    }
};

#endif