#ifndef SNAPSHOTMANAGER_H
#define SNAPSHOTMANAGER_H

#include "../partial.h"

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
#include <chrono>

struct MetricValue 
{
    std::shared_ptr<Metric> metric;    
    uint64_t value;     
    std::string name;   
    std::string unit;   
};

struct ProfilingSnapshot 
{
    std::vector<MetricValue> metrics;
    uint64_t timestamp_ms;             
    uint64_t duration_ms; 
    
    friend std::ostream& operator<<(std::ostream& ostr, const ProfilingSnapshot& snapshot)
    {
        for(const auto& metric : snapshot.metrics)
        {
            ostr << metric.name << ": " << metric.value << std::endl;
        }
        return ostr;
    }
};

class SnapshotManager
{
    std::map<std::string, uint64_t> last_values;
    std::vector<ProfilingSnapshot> snapshots;

    public:
    ProfilingSnapshot collect_snapshot(uint64_t duration_ms, snapshotData& data)
    {
        ProfilingSnapshot snapshot;

        snapshot.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();//абсолютное время собираемого снапшота
        snapshot.duration_ms = duration_ms;
        
        for (const auto& event : data)
        {
            uint64_t current_value = event.second;
            Metric* metric = event.first.get();
            uint64_t last_value = last_values[metric->name_];
            
            uint64_t delta = current_value - last_value;
            last_values[metric->name_] = current_value;
            
            MetricValue metric_value;
            metric_value.metric = event.first;
            metric_value.value = delta;
            metric_value.name = metric->name_;
            metric_value.unit = metric->unit_;
            
            snapshot.metrics.push_back(metric_value);
        }

        snapshots.push_back(snapshot);

        return snapshot;
    }

    const std::vector<ProfilingSnapshot>& getSnapshots()
    {
        return snapshots;
    }
};

#endif