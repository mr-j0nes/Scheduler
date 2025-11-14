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
target_compile_options(Scheduler_test PRIVATE -Wall -Werror)
target_link_libraries(Scheduler_test Threads::Threads gtest gtest_main)

add_test(NAME Scheduler_test COMMAND Scheduler_test)

# Note to contributors
message("
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Important Note for Contributors: This repository uses automated releases. 
   Please follow the Conventional Commits specifications to write the correct 
   commit messages, ensuring the appropriate release process is triggered. To 
   enforce this on *nix environments, you can use our commit-msg hook:
   
      git config core.hooksPath \"<path_to_this_repo>/.githooks\"
   
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
")
