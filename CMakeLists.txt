cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем все исходные файлы
add_executable(my_program
    main.cpp
    manager/manager.cpp
    manager/manager.h
    metrics/metrics_collector.cpp
    metrics/metrics_collector.h
    processes/process_manager.cpp
    processes/process_manager.h
    console_interface/user_interface_c.cpp
    console_interface/user_interface_c.h
)

# Включаем директории с заголовками
target_include_directories(my_program PRIVATE
    manager
    metrics
    processes
    console_interface
)