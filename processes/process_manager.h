#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <unistd.h>
#include <sys/wait.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <atomic>

class ProcessManager
{
    std::atomic<pid_t> child_pid{-1};
    std::thread waiter;
    std::atomic<bool> is_run{false};

    void report_error(const std::string& error);

    public:
    ProcessManager() = default;
    ~ProcessManager()
    {
        terminate_process();
        if(waiter.joinable())
        {
            waiter.join();
        }
    }

    pid_t launch_programm(const std::string& programm, const std::vector<std::string>& args);
    void terminate_process();
    bool is_running();
    void wait_child_process();
    pid_t get_pid();
};

#endif