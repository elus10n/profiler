#include "process_manager.h"

pid_t ProcessManager::launch_programm(const std::string& programm, const std::vector<std::string>& args)
{
    if(programm.empty())
    {
        report_error("From PM: programm path is empty!");
        return -1;
    }

    if(is_running())
    {
        report_error("From PM: is already running!");
        return -1;
    }

    child_pid = fork();

    if(child_pid == -1)
    {
        report_error("From PM: error with fork()!");
        return -1;
    }
    else if(child_pid == 0)//дочерний
    {
        std::vector<char*> argv;
        char* programm_copy = strdup(programm.c_str());
        argv.push_back(programm_copy);
        for(const auto& arg : args)
        {
            argv.push_back(strdup(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(programm.c_str(), argv.data());

        for (auto& arg : argv) 
        {
            free(arg);
        }
        std::cerr << "From PM: error with execvp(...): " << std::endl;
        exit(EXIT_FAILURE);
    }

    return child_pid;
}

int ProcessManager::wait_for_child_process()
{
    if(!is_running())
    {
        return -1;
    }

    int status;
    waitpid(child_pid,&status,0);
    child_pid = -1;
    if (WIFEXITED(status)) 
    {
        return WEXITSTATUS(status); 
    } 
    else 
    {
        return -1;
    }
}

void ProcessManager::report_error(const std::string& error)
{
    if(callback_)
    {
        callback_(error);
    }
    else
    {
        std::cerr << error;
    }
}

void ProcessManager::setup_error_callback(error_callback callback)
{
    callback_ = callback;
}

void ProcessManager::terminate_process()
{
    if(is_running())
    {
        kill(child_pid,SIGTERM);
        child_pid = -1;
    }
}

bool ProcessManager::is_running() const
{
    return child_pid > 0;
}

pid_t ProcessManager::get_pid() const
{
    return child_pid;
}