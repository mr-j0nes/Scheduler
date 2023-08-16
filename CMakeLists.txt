cmake_minimum_required(VERSION 3.7)
project(Scheduler)

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

# Note to contributors
message("
!! Important Note for Contributors: Your adherence to our contribution guidelines,
!! including the Conventional Commits format for commit messages, is crucial to
!! our automated release process.
!! 
!! To facilitate this, if you work on a *nix environment and if you have access to 
!! the bash shell, you have the option to manually configure the enforcement of
!! the Conventional Commits message format. Simply run the following command in
!! your terminal from the project's root directory:
!! 
!!     git config core.hooksPath '<path_to_this_repo>/.githooks'
!! 
!! By setting up a custom Git hooks path, you ensure uniform commit message
!! formatting, enabling us to maintain an organized development history and to
!! automatically generate reliable releases.
!! 
!! Your help is really apprecated.
")
