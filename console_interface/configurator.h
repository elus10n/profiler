#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

//приглашение ко вводу перенести на следующую строку там, где этого не сделано

#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <limits>
#include <sstream>
#include <map>

#include "../metrics/metric_types.h"
#include "text_printer.h"

const int modes_count = 4;

const int min_interval = 100;
const int max_interval = 5000;

const int metrics_count = 7;

std::map<int, MetricType> metric_map = 
{
    {1, MetricType::CPU_CYCLES},
    {2, MetricType::INSTRUCTIONS},
    {3, MetricType::CACHE_MISSES},
    {4, MetricType::BRANCH_MISSES},
    {5, MetricType::CACHE_REFERENCES},
    {6, MetricType::PAGE_FAULTS},
    {7, MetricType::CONTEXT_SWITCHES}
};

enum class Modes
{
    MONITORING, WORKLOAD, PHASE, CORRELATION, NONE
};

struct Configuration
{
    Modes mode;
    std::vector<MetricType> metrics = {MetricType::PAGE_FAULTS};
    int interval_ms = 500;
    std::string program_name;
    std::vector<std::string> program_args;

    friend std::ostream& operator<<(std::ostream& ostr, const Configuration& cfg)
    {
        ostr << cfg.interval_ms << " " << cfg.program_name << std::endl;
        for(const auto& arg : cfg.program_args)
        {
            ostr << arg << " ";
        }
        ostr << std::endl;
        return ostr;
    }
};

class Configurator
{
    static void clean_cin()
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    static Modes mode_select()
    {
        TextPrinter::modes_print();

        std::cout << "Select operating mode!" << std::endl;
    
        int choice = 0;

        while(true)
        {
            std::cout << ">";
            std::cin >> choice;
            if(choice >= 1 && choice <= modes_count)
            {
                break;
            }

            std::cout << "Incorrect input. Please select from 1 to " << modes_count << std::endl;
            clean_cin();
        }

        switch(choice)
        {
            case 1 : return Modes::MONITORING;
            case 2 : return Modes::WORKLOAD;
            case 3 : return Modes::PHASE;
            case 4 : return Modes::CORRELATION;
            default : throw std::runtime_error("Incorrect mode in Configurator::mode_select()!");
        }

        return Modes::NONE;
    }

    static Configuration configurate_mode(Modes mode)
    {
        Configuration config;

        config.mode = mode;
        config.interval_ms = get_interval();
        config.program_name = get_program_path();
        config.program_args = get_program_args();

        switch(mode)
        {
            case Modes::MONITORING:
            {
                config.metrics = get_metrics();
                return config;
            }
            case Modes::WORKLOAD:
            {
                //Это временно
                //CPU_BOUND - cycles, instructions, cache-misses, ....
                //MEMORY_BOUND - cycles, instructions, cache-misses, ....
                //IO_BOUND - cycles, instructions, .....
                //BRANCH_BOUND - branch-misses, ....
                config.metrics = {MetricType::BRANCH_MISSES, MetricType::CACHE_MISSES,MetricType::CPU_CYCLES,MetricType::INSTRUCTIONS};
                return config;
            }
            case Modes::PHASE:
            {
                config.metrics = {MetricType::BRANCH_MISSES, MetricType::CACHE_MISSES,MetricType::CPU_CYCLES,MetricType::INSTRUCTIONS};
                return config;
            }
            case Modes::CORRELATION:
            {
                config.metrics = get_metric_pair();
                return config;
            }
            default : throw std::runtime_error("Incorrect mode in Configurator::configurate_mode()!");
        }

        return config;
    }

    static std::string get_program_path()
    {
        std::string path = "";
        
        std::cout << "Enter the path to the program!" << std::endl;
        clean_cin();
        std::cout << ">";

        std::cin >> path;
        
        return path;
    }

    static std::vector<std::string> get_program_args()
    {
        std::string line = "";
        std::vector<std::string> args;
        std::string arg = "";

        std::cout << "Enter an program args separated by spaces" << std::endl;
        clean_cin();
        std::cout << ">";

        std::getline(std::cin,line);

        std::cout << std::endl;

        std::istringstream istr_a(line);
        while(istr_a >> arg)
        {
            args.push_back(arg);
        }
        
        return args;
    }

    static int get_interval()
    {
        std::cout << "Select the metrics collection interval (ms)!" << std::endl;
    
        int interval = 0;

        while(true)
        {
            clean_cin();
            std::cout << ">";
            std::cin >> interval;
            if(interval >= min_interval && interval <= max_interval)
            {
                break;
            }

            std::cout << "Incorrect input. Please select from " << min_interval << " to " << max_interval << std::endl;
        }

        return interval;
    }

    static std::vector<MetricType> get_metric_pair()
    {
        TextPrinter::metrics_print();

        std::vector<MetricType> metrics_pair;
        std::string line = "";

        std::cout << "Enter a couple of metrics for analysis, separated by spaces!" << std::endl;
        while(true)
        {
            metrics_pair.clear();

            std::cout << ">";
            std::getline(std::cin,line);
            std::stringstream stream(line);

            MetricType metric;
            int raw_metric = 0;

            stream >> raw_metric;
            if(raw_metric < 1 || raw_metric > metrics_count)
            {
                std::cout << "Incorrect input. Please select from 1 to " << metrics_count << std::endl;
                continue;
            }
            metric = metric_map[raw_metric];
            metrics_pair.push_back(metric);
            
            stream >> raw_metric;
            if(raw_metric < 1 || raw_metric > metrics_count)
            {
                std::cout << "Incorrect input. Please select from 1 to " << metrics_count << std::endl;
                continue;
            }
            metric = metric_map[raw_metric];
            metrics_pair.push_back(metric);

            if(stream >> raw_metric)
            {
                std::cout << "More than 2 input parameters were found. They will be discarded!" << std::endl;
            }
            break;
        }
        return metrics_pair;
    }

    static std::vector<MetricType> get_metrics()
    {
        TextPrinter::metrics_print();

        std::vector<MetricType> metrics;
        std::string line = "";

        std::cout << "Enter metrics for analysis, separated by spaces!" << std::endl;

        std::getline(std::cin,line);

        std::cout << std::endl;
        std::stringstream stream(line);
        int raw_metric = 0;

        bool once_detected = false;
        while(stream >> raw_metric)
        {
            MetricType metric;

            if(raw_metric < 1 || raw_metric > metrics_count)
            {
                if(!once_detected)
                {
                    std::cout << "Invalid input detected. This and subsequent inputs will be discarded!" << std::endl;
                    once_detected = true;
                }
                continue;
            }

            metric = metric_map[raw_metric];
            metrics.push_back(metric);
        }

        if(metrics.empty())
        {
            return {MetricType::INSTRUCTIONS};
        }

        return metrics;
    }

    public:

    static Configuration get_configuration()
    {
        TextPrinter::welcome_print();

        Modes mode;
        mode = mode_select();

        Configuration config = configurate_mode(mode);

        return config;
    }
};

#endif