#include <iostream>
#include <functional>
#include <vector>
#include <unordered_map>

#include "metrics/metrics_collector.h"

const bool is_counting = false;

int main(int argc, char** argv) 
{
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
        std::vector<MetricType> hotspot_vec = {MetricType::CPU_CYCLES};
        ProfilingConfiguration hotspot_cfg(Modes::CPU_HOTSPOT, hotspot_vec, 100);

        collector.start_profiling(pid, hotspot_cfg);
        sleep(20);
        collector.stop_profiling();

        std::unordered_map<std::vector<uint64_t>, int, CallchainHash> callchains = collector.get_hotspot_data();
        for(auto& [vec, count] : callchains)
        {
            if(count > 500)
            {
                for(int i = 0;i < vec.size()/ 4; i++)
                    std::cout << vec[i] << " <- ";
                std::cout << " count: " << count << std::endl;
            }
        }
    }
}