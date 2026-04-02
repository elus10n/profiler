cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем все исходные файлы
add_executable(my_program
    main.cpp
    metrics/metrics_collector.cpp
    metrics/metrics_collector.h
    metrics/EventCreator.h
    metrics/SnapshotManager.h
    processes/process_manager.cpp
    processes/process_manager.h
    partial.h
)

# Включаем директории с заголовками
target_include_directories(my_program PRIVATE
    metrics
    processes
)