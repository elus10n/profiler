#include "manager.h"

Manager::Manager()
{
    manager = std::make_unique<ProcessManager>();
    collector = std::make_unique<MetricCollector>();

    setup();
}

bool Manager::start_profiling(const std::string& programm, const std::vector<std::string>& args, const ProfilingConfiguration& config)
{
    if(manager->get_pid() != -1)
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
        report_error("Profiling Configuration is invalid!");
        return false;
    }
    current_config = config;

    if(manager->launch_programm(programm,args) == -1)
    {
        report_error("Problem with fork");
        return false;
    }

    if(!manager->is_running())
    {
        report_error("Process ends after start! Check program path!");
        return false;
    }

    if(!collector->start_profiling(manager->get_pid(), current_config.metrics, current_config.interval_ms))
    {
        manager->terminate_process();
        return false;
    }

    return true;
}

void Manager::setup()
{
    metric_callback metric_callback = [this](const ProfilingSnapshot& snapshot)
    {
        this->on_metrics_recieved(snapshot);
    };

    error_callback error_callback = [this](const std::string& error)
    {
        this->on_error_recieved(error);
    };

    log_callback log_callback = [this](const std::string& log)
    {
        this->on_log_recieved(log);
    };

    collector->setup_metric_callback(metric_callback);
    collector->setup_error_callback(error_callback);
    collector->setup_log_callback(log_callback);
}

void Manager::stop_profiling()
{
    collector->stop_profiling();
    manager->terminate_process();
}

void Manager::setup_metrics_callback(metric_callback callback)
{
    callback_metric = callback;
}

void Manager::setup_error_callback(error_callback callback)
{
    callback_error = callback;
}

void Manager::setup_log_callback(log_callback callback)
{
    callback_log = callback;
}

bool Manager::is_active() const
{
    return collector->is_profiling();
}

bool Manager::is_process_alive() const
{
    return manager->is_running();
}

pid_t Manager::get_current_pid() const
{
    return manager->get_pid();
}

std::string Manager::get_current_programm() const
{
    return current_programm;
}

ProfilingConfiguration Manager::get_current_config() const
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

void Manager::on_log_recieved(const std::string& log)
{
    report_log(log);
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

void Manager::report_log(const std::string& log)
{
    if(callback_log)
    {
        callback_log(log);
    }
    else
    {
        report_error("Undefined callback in Manager!");
    }
}

std::ostream& operator<<(std::ostream& ostr, const ProfilingConfiguration pr_config)
{
    ostr << "Metrics: (later)" << std::endl;
    ostr << "Profiling_interval: "<<pr_config.interval_ms<<std::endl;
    return ostr;
}