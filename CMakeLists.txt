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
    demangler/demangler.h
    demangler/symbol_resolver.h
    partial.h
)

# Включаем директории с заголовками
target_include_directories(my_program PRIVATE
    metrics
    processes
    demangler
)

# --- ВОТ ЭТО НУЖНО ДОБАВИТЬ ---

# Ищем библиотеку libelf в системе
find_library(LIBELF_LIB elf)

if(NOT LIBELF_LIB)
    message(FATAL_ERROR "libelf not found! Install it with: sudo apt-get install libelf-dev")
endif()

# Линкуем библиотеку к нашему проекту
target_link_libraries(my_program PRIVATE ${LIBELF_LIB})

# ------------------------------