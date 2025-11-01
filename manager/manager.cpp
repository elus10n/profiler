#include "manager.h"

Manager::Manager()
{
    manager = std::make_unique<ProcessManager>();
    collector = std::make_unique<MetricCollector>();
}

bool Manager::setup_and_start_profiling(const std::string& programm, const std::vector<std::string>& args, const Configuration& config)//надо пересмотреть, как ошибки будут доходить
{
    if(current_pid != -1)
    {
        manager->terminate_process();
    }

    if(programm.empty())
    {
        report_error("Programm path is empty!");
        return false;
    }
    current_programm = programm;

    if(!config.is_valid())
    {
        report_error("Configuration is invalid!");
        return false;
    }
    current_config = config;

    pid_t new_pid = manager->launch_programm(programm,args);

    if(new_pid == -1)
    {
        report_error("Failed with process create!");
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if(!manager->is_running())
    {
        report_error("Process ends after start!");
        return false;
    }

    current_pid = new_pid;

    metric_callback metric_callback = [this](const ProfilingSnapshot& snapshot)
    {
        this->on_metrics_recieved(snapshot);
    };

    error_callback error_callback = [this](const std::string& error)
    {
        this->on_error_recieved(error);
    };

    collector->setup_metric_callback(metric_callback);
    collector->setup_error_callback(error_callback);


    if(!collector->start_profiling(current_pid, current_config.metrics, current_config.interval_ms))
    {
        manager->terminate_process();
        return false;
    }
    is_profiling_active = true;

    return true;
}

void Manager::stop_profiling()
{
    bool expected = true;
    if (!is_profiling_active.compare_exchange_strong(expected, false)) {
        report_error("Profiling inactive already!");
        return;
    }
    
    collector->stop_profiling();
    manager->terminate_process();
    
    current_pid = -1;
    current_programm = "idle";
    current_config = default_cfg;
}

void Manager::setup_metrics_callback(metric_callback callback)
{
    callback_metric = callback;
}

void Manager::setup_error_callback(error_callback callback)
{
    callback_error = callback;
}

bool Manager::is_active() const
{
    return is_profiling_active;
}

pid_t Manager::get_current_pid() const
{
    return current_pid;
}

std::string Manager::get_current_programm() const
{
    return current_programm;
}

Configuration Manager::get_current_config() const
{
    return current_config;
}

void Manager::on_metrics_recieved(const ProfilingSnapshot& snapshot)
{
    report_metrics(snapshot);
}

void Manager::on_error_recieved(const std::string& error)
{
    report_error(error);
}

void Manager::report_error(const std::string& error)
{
    if(callback_error)
    {
        callback_error(error);
    }
    else
    {
        std::cerr << error;
    }
}

void Manager::report_metrics(const ProfilingSnapshot& snapshot)
{
    if(callback_metric)
    {
        callback_metric(snapshot);
    }
    else
    {
        report_error("Unfefined callback in Manager!");
    }
}