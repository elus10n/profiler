#include <iostream>
#include <functional>
#include <vector>
#include <unordered_map>

#include "metrics/metrics_collector.h"
#include "demangler/symbol_resolver.h"

const bool is_counting = false;

int main(int argc, char** argv) 
{
    if (argc < 2) return 1;
    MetricCollector collector;
    int pid = atoi(argv[1]);

    auto ErrorCallback = [](const std::string& error)
    {
        std::cout << "Error: " << error << std::endl;
    };

    auto LogCallback = [](const std::string& log)
    {
        std::cout << "Log: " << log << std::endl;
    };

    collector.setup_error_callback(ErrorCallback);
    collector.setup_log_callback(LogCallback);

    if(is_counting)
    {
        std::vector<MetricType> counting_vec = {MetricType::INSTRUCTIONS, MetricType::CPU_CYCLES};
        ProfilingConfiguration counting_cfg(Modes::COUNTING, counting_vec, 500);

        auto MetricCallback = [](const ProfilingSnapshot& snapshot)
        {
            std::cout << "MetricCallback is here!" << std::endl;
            for(const auto& metric : snapshot.metrics)
                std::cout << metric.name << ": " << metric.value << std::endl;
        };

        collector.setup_metric_callback(MetricCallback);

        collector.start_profiling(pid, counting_cfg);
        sleep(5);
        collector.stop_profiling();
    }
    else
    {
        std::vector<MetricType> hotspot_vec = {MetricType::CACHE_MISSES};
        ProfilingConfiguration hotspot_cfg(Modes::C_M_HOTSPOT, hotspot_vec, 100);

        collector.start_profiling(pid, hotspot_cfg);
        sleep(20);
        collector.stop_profiling();

        SymbolResolver resolver(pid);
        auto callchains = collector.get_hotspot_data();

        std::vector<std::pair<std::vector<uint64_t>, int>> sorted_chains(callchains.begin(), callchains.end());

        std::sort(sorted_chains.begin(), sorted_chains.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

        size_t top_n = std::min(sorted_chains.size(), (size_t)5);
        for(size_t i = 0; i < top_n; i++)
        {
            auto& [vec, count] = sorted_chains[i];
            
            for(size_t j = 0; j < vec.size(); j++)
            {
                std::cout << resolver.resolve(vec[j]);
                if (j < vec.size() - 1) std::cout << " <- ";
            }
            std::cout << " count: " << count << std::endl;
        }
    }
    return 0;
}