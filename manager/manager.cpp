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
        if(callback_error)
        {
            callback_error("Programm path is empty!");
        }
        return -1;
    }
    current_programm = programm;

    if(!config.is_valid())
    {
        if(callback_error)
        {
            callback_error("Configuration is invalid!");
        }
        return -1;
    }
    current_config = config;

    manager->launch_programm(programm, args);
    current_pid = manager->get_pid();

    metric_callback metric_callback = [this](const ProfilingSnapshot& snapshot)
    {
        on_metrics_recieved(snapshot);
    };

    error_callback error_callback = [this](const std::string& error)
    {
        on_error_recieved(error);
    };

    collector->setup_metric_callback(metric_callback);
    collector->setup_error_callback(error_callback);

    collector->start_profiling(current_pid, current_config.metrics, current_config.interval_ms);
}

bool Manager::stop_profiling()
{
    if(!is_profiling_active())
    {
        report_error("Profiling inactive alredy!");
        return false;
    }

    collector->stop_profiling();
    is_profiling = false;
}

void Manager::setup_metrics_callback(metric_callback callback)
{
    callback_metric = callback;
}

void Manager::setup_error_callback(error_callback callback)
{
    callback_error = callback;
}

bool Manager::launch_programm(const std::string& programm, const std::vector<std::string> args)
{

}

bool Manager::is_profiling_active() const
{

}

pid_t Manager::get_current_pid() const
{

}

std::string Manager::get_current_programm() const
{

}

Configuration Manager::get_current_config() const
{

}

void Manager::on_metrics_recieved(const ProfilingSnapshot& snapshot)
{

}

void Manager::on_error_recieved(const std::string& error)
{

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