#include <iostream>
#include <functional>

#include "manager/manager.h"


int main() 
{
    metric_callback metric_callback = [](const ProfilingSnapshot& snapshot)
    {
        std::cout << "Metric!" << std::endl;
    };

    error_callback error_callback = [](const std::string& error)
    {
        std::cout << "Error: "<<error << std::endl;
    };

    Manager manager;
    manager.setup_error_callback(error_callback);
    manager.setup_metrics_callback(metric_callback);
    Configuration cfg;
    manager.setup_and_start_profiling("test_programm/test",{}, cfg);
}