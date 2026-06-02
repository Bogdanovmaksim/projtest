cmake_minimum_required(VERSION 3.20)

project(TaskManagerCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(task_core
        src/task.cpp
        src/task_manager.cpp
)
target_include_directories(task_core PUBLIC include)

add_executable(task_manager_app src/main.cpp)
target_link_libraries(task_manager_app PRIVATE task_core)

enable_testing()

add_executable(task_manager_tests
    tests/test_main.cpp
    tests/test_task_core.cpp
    tests/test_task_manager.cpp
)
target_link_libraries(task_manager_tests PRIVATE task_core)
add_test(NAME task_manager_tests COMMAND task_manager_tests)
