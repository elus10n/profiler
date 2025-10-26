#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <unistd.h>
#include <sys/wait.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <functional>

using error_callback = std::function<void(const std::string& error)>;

class ProcessManager
{
    pid_t child_pid = -1;

    error_callback callback_;

    void report_error(const std::string& error);

    public:
    ProcessManager() = default;
    ~ProcessManager() = default;

    pid_t launch_programm(const std::string& programm, const std::vector<std::string>& args);
    int wait_for_child_process();
    void terminate_process();
    bool is_running() const;
    pid_t get_pid() const;
    void setup_error_callback(error_callback callback);
};

#endif