cmake_minimum_required(VERSION 3.7)
project(TaskScheduler)

set(CMAKE_CXX_STANDARD 11)

include_directories(externals/croncpp/include externals/CTPL externals/googletest)

# threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add the subdirectory for Google Test
add_subdirectory(externals/googletest)

add_executable(example example.cpp)
target_link_libraries(example Threads::Threads)

enable_testing()

add_executable(Scheduler_test Scheduler_test.cpp)
target_link_libraries(Scheduler_test Threads::Threads gtest gtest_main)

add_test(NAME Scheduler_test COMMAND Scheduler_test)

