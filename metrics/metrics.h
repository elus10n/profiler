#ifndef METRICS_H
#define METRICS_H

#include <string>
#include <linux/perf_event.h>
#include <variant>

class Metric
{
    public:
    perf_type_id type_;
    std::string name_;
    std::string unit_;
    unsigned long long config_;

    Metric(perf_type_id type, std::string name, std::string unit, unsigned long long config) : type_(type), name_(name), unit_(unit), config_(config) {}
};

class Instructions: public Metric
{
    public:

    Instructions() : Metric(PERF_TYPE_HARDWARE, "instructions", "count", PERF_COUNT_HW_INSTRUCTIONS) {}
};

class CpuCycles: public Metric
{
    public:

    CpuCycles() : Metric(PERF_TYPE_HARDWARE, "cpu_cycles", "cycles", PERF_COUNT_HW_CPU_CYCLES) {}
};

class CacheMisses: public Metric
{
    public:

    CacheMisses() : Metric(PERF_TYPE_HARDWARE, "cache_misses", "misses", PERF_COUNT_HW_CACHE_MISSES) {}
};

class CacheReferences: public Metric
{
    public:

    CacheReferences() : Metric(PERF_TYPE_HARDWARE, "cache_references", "references", PERF_COUNT_HW_CACHE_REFERENCES) {}
};

class BranchMisses: public Metric
{
    public:

    BranchMisses() : Metric(PERF_TYPE_HARDWARE, "branch_misses", "misses", PERF_COUNT_HW_BRANCH_MISSES) {}
};

class PageFaults: public Metric
{
    public:

    PageFaults() : Metric(PERF_TYPE_SOFTWARE, "page_faults", "faults", PERF_COUNT_SW_PAGE_FAULTS) {}
};

class ContextSwithces: public Metric
{
    public:

    ContextSwithces() : Metric(PERF_TYPE_SOFTWARE, "context_swithces", "swithces", PERF_COUNT_SW_CONTEXT_SWITCHES) {}
};


#endif