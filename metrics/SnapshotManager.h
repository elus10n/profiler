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
#include <unordered_map>
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

struct CallchainHash 
{
    std::size_t operator()(const std::vector<uint64_t>& v) const 
    {
        std::size_t seed = v.size();
        for(auto& i : v) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

class SnapshotManager
{
    std::map<std::string, uint64_t> last_values;
    std::vector<ProfilingSnapshot> snapshots;

    std::unordered_map<std::vector<uint64_t>, int, CallchainHash> callchains;

    void copy_from_ring(const HotspotRawData& raw, uint64_t start, void* dest, size_t size) 
    {
        uint8_t* d = static_cast<uint8_t*>(dest);
        
        uint64_t offset = start & raw.data_mask;
        
        uint64_t size_to_end = (raw.data_mask + 1) - offset;

        if (size <= size_to_end) 
        {
            std::memcpy(d, raw.data_start + offset, size);
        } 
        else 
        {
            std::memcpy(d, raw.data_start + offset, size_to_end);
            std::memcpy(d + size_to_end, raw.data_start, size - size_to_end);
        }
    }

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

    const std::vector<ProfilingSnapshot>& getSnapshots() {return snapshots;}

    void append_callchain(const HotspotRawData& data)
    {
        if (data.head == data.tail) return;

        uint64_t current = data.tail;

        while (current < data.head) 
        {
            perf_event_header header;
            copy_from_ring(data, current, &header, sizeof(header));

            if (header.size == 0) break; 

            if (header.type == PERF_RECORD_SAMPLE) 
            {
                uint64_t payload_offset = current + sizeof(perf_event_header);

                uint64_t ip;//инструкция, на которой попались
                copy_from_ring(data, payload_offset, &ip, sizeof(ip));
                payload_offset += sizeof(ip);

                uint64_t nr;
                copy_from_ring(data, payload_offset, &nr, sizeof(nr));
                payload_offset += sizeof(nr);

                std::vector<uint64_t> callchain(nr + 1);//стек вызовов
                callchain[0] = ip;
                if (nr > 0) 
                    copy_from_ring(data, payload_offset, callchain.data() + 1, nr * sizeof(uint64_t));
                callchains[callchain]++;
            }

            current += header.size;
        }
    }

    std::unordered_map<std::vector<uint64_t>, int, CallchainHash> get_callchains() {return callchains;}
};

#endif