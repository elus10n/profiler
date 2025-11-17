#include "process_manager.h"

pid_t ProcessManager::launch_programm(const std::string& programm, const std::vector<std::string>& args)
{
    if(programm.empty())
    {
        return -1;
    }

    if(is_running())
    {
        return -1;
    }

    child_pid = fork();

    if(child_pid.load() == -1)
    {
        return -1;
    }
    else if(child_pid.load() == 0)
    {
        std::vector<char*> argv;
        char* programm_copy = strdup(programm.c_str());
        argv.push_back(programm_copy);
        for(const auto& arg : args)
        {
            argv.push_back(strdup(arg.c_str()));
        }
        argv.push_back(nullptr);

        std::cerr << execvp(programm.c_str(), argv.data()) << std::endl;

        for (auto& arg : argv) 
        {
            free(arg);
        }
        exit(EXIT_FAILURE);
    }
    else
    {
        is_run = true;
        waiter = std::thread(&ProcessManager::wait_child_process,this);
    }
    return child_pid.load();
}

void ProcessManager::terminate_process()
{
    if(is_running())
    {
        kill(child_pid,SIGTERM);
    }
}

void ProcessManager::wait_child_process()
{
    int status;
    pid_t ret_pid = waitpid(child_pid.load(),&status,0);
    if(ret_pid == child_pid.load())
    {
        is_run = false;
        child_pid = -1;
    }
}

bool ProcessManager::is_running()
{   
    return is_run.load();
}

pid_t ProcessManager::get_pid()
{   
    return child_pid.load();
}
