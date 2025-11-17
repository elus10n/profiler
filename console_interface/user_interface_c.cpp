#include "user_interface_c.h"

ConsoleInterface::ConsoleInterface()
{
    manager = std::make_unique<Manager>();

    setup_callbacks();
}

void ConsoleInterface::run()
{
    std::cout << "This is a profiler, here you can analyze the performance of your program!" << std::endl;

    working_loop();

    std::lock_guard<std::mutex> lock(new_data);
    if(error_recieved)
    {
        print_error_screen();
    }
    if(new_data_available)
    {
        print_screen();
    }

    std::cout << "Have a nice day!"<< std::endl;
}

void ConsoleInterface::working_loop()
{
    Configuration cfg = get_configuration();
    if(!manager->start_profiling(cfg.program_name, cfg.program_args, cfg.cfg))
    {
        return;
    }

    std::thread wait_thread(&ConsoleInterface::wait_for_user_input,this);
    while(manager->is_process_alive() && !stop_signal)
    {
        std::lock_guard<std::mutex> lock(new_data);
        if(error_recieved)
        {
            print_error_screen();
        }
        if(new_data_available)
        {
            print_screen();
        }
        error_recieved = false;
        new_data_available = false;
    }

    if(wait_thread.joinable())
    {
        wait_thread.detach();
    }

    manager->stop_profiling();
}

void ConsoleInterface::wait_for_user_input()
{
    char input;
    std::cin >> input;
    if(input == 'q')
    {
        stop_signal = true;
    }  
}

void ConsoleInterface::setup_callbacks()
{
    MetricCallback metric_callback = [this](const ProfilingSnapshot& snapshot)
    {
        this->on_metric_recv(snapshot);
    };

    ErrorCallback error_callback = [this](const std::string& error)
    {
        this->on_error_recv(error);
    };

    LogCallback log_callback = [this](const std::string& log)
    {
        this->on_log_recv(log);
    };

    manager->setup_metrics_callback(metric_callback);
    manager->setup_error_callback(error_callback);
    manager->setup_log_callback(log_callback);
}

Configuration ConsoleInterface::get_configuration() const //этот ужас потом необходимо вынести в отдельный класс
{
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║         PERFORMANCE METRICS          ║\n";
    std::cout << "╠══════════════════════════════════════╣\n";
    std::cout << "║  1. Instructions                     ║\n";
    std::cout << "║  2. CPU Cycles                       ║\n";
    std::cout << "║  3. Cache Misses                     ║\n";
    std::cout << "║  4. Cache References                 ║\n";
    std::cout << "║  5. Branch Misses                    ║\n";
    std::cout << "║  6. Page Faults                      ║\n";
    std::cout << "║  7. Context Switches                 ║\n";
    std::cout << "║  0. Select All Metrics               ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    std::cout << "Enter your choice(s) separated by spaces: ";

    size_t choice;
    MetricType metric;
    std::vector<MetricType> metrics;
    Configuration config;

    bool flag = true;
    std::string line;
    std::getline(std::cin,line);
    std::istringstream istr_m(line);
    while(istr_m >> choice)
    {   
        if(choice < 0 || choice > 7)
        {
            std::cout << std::endl;
            std::cout << "Incorrect metric choice: "<< choice << std::endl;

            flag = false;
            break;
        }
        switch(choice)
        {
            case 1:
            {
                metric = MetricType::INSTRUCTIONS;
                break;
            }
            case 2:
            {
                metric = MetricType::CPU_CYCLES;
                break;
            }
            case 3:
            {
                metric = MetricType::CACHE_MISSES;
                break;
            }
            case 4:
            {
                metric = MetricType::CACHE_REFERENCES;
                break;
            }
            case 5:
            {
                metric = MetricType::BRANCH_MISSES;
                break;
            }
            case 6:
            {
                metric = MetricType::PAGE_FAULTS;
                break;
            }
            case 7:
            {
                metric = MetricType::CONTEXT_SWITCHES;
                break;
            }
            case 0:
            {
                for(int i = static_cast<int>(MetricType::INSTRUCTIONS);i <= static_cast<int>(MetricType::CONTEXT_SWITCHES);i++)
                {
                    metrics.push_back(static_cast<MetricType>(i));
                }
                break;
            }
            default:
            {
                throw std::runtime_error("Incorrect metric type!");
                break;
            }
        }
        if(choice == 0)
        {
            break;
        }
        metrics.push_back(metric);
    }
    config.cfg.metrics = metrics;

    int interval;
    std::cout << "Enter an profiling interval(ms) : 100 - 5000 >";
    while(true)
    {
        std::cin >> choice;
        if(choice >= 100 && choice <= 5000)
        {
            break;
        }
        std::cout << std::endl;
        std::cout << "Incorrect input!"<< std::endl;
        std::cout << ">";

        clean_cin();
    }
    interval = choice;
    config.cfg.interval_ms = interval;
    clean_cin();

    std::string program_name;
    std::cout << "Enter a path to program >";
    std::cin >> program_name;
    config.program_name = program_name;
    clean_cin();

    std::string program_arg;
    std::vector<std::string> program_args;
    std::cout << "Enter an program args separated by spaces >";
    std::getline(std::cin,line);
    std::istringstream istr_a(line);
    while(istr_a >> program_arg)
    {
        program_args.push_back(program_arg);
    }
    config.program_args = program_args;

    return config;
}

void ConsoleInterface::print_screen()//потом разделить на print_log и print_metrics
{
    system("clear");
    std::cout << "LOGS:" << std::endl;
    for(const auto& log : logs)
    {
        std::cout << log << std::endl;
    }

    std::cout<<"Snapshot:" << std::endl;
    std::cout << last_snapshot << std::endl;

    print_configuration();
}

void ConsoleInterface::print_error_screen()
{
    system("clear");
    std::cout << "ERROR: " << last_error << std::endl;
}

void ConsoleInterface::on_metric_recv(const ProfilingSnapshot& snapshot)
{
    std::lock_guard<std::mutex> lock(new_data);
    new_data_available = true;
    last_snapshot = snapshot;
}

void ConsoleInterface::on_error_recv(const std::string& error)
{
    std::lock_guard<std::mutex> lock(new_data);
    error_recieved = true;
    last_error = error;
}

void ConsoleInterface::on_log_recv(const std::string& log)
{
    std::lock_guard<std::mutex> lock(new_data);
    new_data_available = true;
    logs.push_back(log);
    if(logs.size() > 10)
    {
        logs.erase(logs.begin());
    }
}

void ConsoleInterface::print_configuration() const
{
    std::cout << "Configuration: "<< std::endl;
    std::cout << manager->get_current_config();
}

void ConsoleInterface::clean_cin() const
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}