#ifndef USER_INTERFACE_C_H
#define USER_INTERFACE_C_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <iostream>
#include <limits>
#include <sstream>

#include "../manager/manager.h"

using MetricCallback = std::function<void(const ProfilingSnapshot& snapshot)>;
using ErrorCallback = std::function<void(const std::string& error)>;
using LogCallback = std::function<void(const std::string& log)>;

struct Configuration
{
    ProfilingConfiguration cfg;
    std::string program_name;
    std::vector<std::string> program_args;

    friend std::ostream& operator<<(std::ostream& ostr, const Configuration cfg)
    {
        ostr << cfg.cfg << std::endl;
        ostr << "For programm: "<<cfg.program_name<<" ";
        for(const auto& arg : cfg.program_args)
        {
            ostr << arg << " ";
        }
        ostr << std::endl;
        return ostr;
    }
};

class ConsoleInterface
{
    std::unique_ptr<Manager> manager;

    std::atomic<bool> stop_signal{false};

    std::string last_error;
    ProfilingSnapshot last_snapshot;
    std::vector<std::string> logs;
    std::atomic<bool> new_data_available{false};
    std::atomic<bool> error_recieved{false};
    std::mutex new_data;

    void on_error_recv(const std::string& error);
    void on_metric_recv(const ProfilingSnapshot& snapshot);
    void on_log_recv(const std::string& log);
    void print_configuration() const;
    void print_screen();
    void print_error_screen();
    void setup_callbacks();
    void wait_for_user_input();

    Configuration get_configuration() const;
    void clean_cin() const;

    void working_loop();

    public:
    ConsoleInterface();
    ~ConsoleInterface() = default;
    void run();
};

#endif