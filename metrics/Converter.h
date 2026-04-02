#ifndef CONVERTER_H
#define CONVERTER_H

#include "metrics.h"
#include "metric_types.h"

#include <memory>
#include <vector>

class Converter
{
    static std::shared_ptr<Metric> convert(const MetricType type)
    {
        switch(type)
        {
            case MetricType::INSTRUCTIONS:
            {
                return std::make_shared<Instructions>();
                break;
            }
            case MetricType::CPU_CYCLES:
            {
                return std::make_shared<CpuCycles>();
                break;
            }
            case MetricType::CACHE_MISSES:
            {
                return std::make_shared<CacheMisses>();
                break;
            }
            case MetricType::CACHE_REFERENCES:
            {
                return std::make_shared<CacheReferences>();
                break;
            }
            case MetricType::BRANCH_MISSES:
            {
                return std::make_shared<BranchMisses>();
                break;
            }
            case MetricType::PAGE_FAULTS:
            {
                return std::make_shared<PageFaults>();
                break;
            }
            case MetricType::CONTEXT_SWITCHES:
            {
                return std::make_shared<ContextSwithces>();
                break;
            }
            default: throw std::runtime_error("Unknown MT in Converter!");
        }
    }

    public:

    static std::vector<std::shared_ptr<Metric>> convert_types_to_metric(const std::vector<MetricType>& types)
    {
        std::vector<std::shared_ptr<Metric>> metrics;
        metrics.resize(types.size());

        int i = 0;
        for(const auto metric : types)
        {
            metrics[i] = convert(metric);
            i++;
        }

        return metrics;
    }
};

#endif