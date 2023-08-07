# Scheduler C++ Library ![Test](https://github.com/mr-j0nes/Scheduler/actions/workflows/ci.yml/badge.svg)

Scheduler is an improved fork of the original [Bosma repository](https://github.com/Bosma/Scheduler), providing a simple and flexible task scheduling framework for C++. The library allows you to schedule tasks to run at specific intervals, specific times, or based on cron expressions. You can also enable or disable tasks dynamically during runtime, as well as removing them via their Id.

## Differences with the original

- Built-in cron was removed in favor of [croncpp](https://github.com/mariusbancila/croncpp) (Cron Expression Parser for C++)
- Namespace changed
- Naming convention changed
- Supported enabling/disabling/removing tasks
- Tasks should be added with a unique Id (task name)

## Features

- Header only library
- Schedule tasks to run at specific intervals or times.
- Support for advance cron-like expressions for task scheduling.
- Enable or disable tasks during runtime.
- Thread-safe task scheduling and management.
- Lightweight and easy to integrate into your C++ projects.
- Task report provides a summary of every task: ID, trigger time and whether it's enabled or not.

## Dependencies

The following dependencies are required for Scheduler:

- [CTPL](https://github.com/vit-vit/CTPL) (C++ Thread Pool Library) [link to CTPL repo]
- [croncpp](https://github.com/mariusbancila/croncpp) (Cron Expression Parser for C++)

Make sure to install them or to include them directly in your project.
In order to be able to run the provided `example.cpp` Please make sure to initialize the submodules by using the `--recurse-submodules` option when cloning the repository:

```bash
git clone --recurse-submodules https://github.com/mr-j0nes/Scheduler.git
```

## Usage

```cpp
#include "Scheduler.hpp"

// Create a scheduler with a desired number of threads
Cppsched::Scheduler scheduler(4);

// Schedule a task to run every 5 seconds
scheduler.every("Task1", std::chrono::seconds(5), []() {
    // Your task code here
    // This task will be executed every 5 seconds, starting in 5 seconds
});

// Schedule a task to run at a specific time
scheduler.at("Task2", "2023-08-01 12:00:00", []() {
    // Your task code here
    // This task will be executed once at the specified time
});

// Schedule a task to run in certain amount of time
scheduler.in("Task3", std::chrono::seconds(8), []() {
    // Your task code here
    // This task will be executed once in 8 seconds
});

// Schedule a task to run with a cron expression
scheduler.cron("Task4", "0 */5 * * * *", []() {
    // Your task code here
    // This task will be executed every 5 minutes, starting at the next minute that is module of 5
});

// Schedule a task to run at intervals without concurrency
scheduler.interval("Task5", std::chrono::seconds(10), []() {
    // Your task code here
    // This task will be executed every 10 seconds, starting in 10 seconds, and no multiple instances will run concurrently
});

// Enable or disable tasks dynamically during runtime
bool d = scheduler.disable_task("Task1");
bool e = scheduler.enable_task("Task1");

// Remove tasks
bool r = scheduler.remove_task("Task1");
```

**Note**: The difference between `interval` and `every` is that multiple instances of a task scheduled with `interval` will never be run concurrently, ensuring that the task is always completed before the next execution. On the other hand, tasks scheduled with `every` will run at the specified interval regardless of the completion time of the previous instance.

## License

Scheduler is released under the [MIT License](LICENSE).

## Contributing

Contributions to Scheduler are welcome! If you find a bug, have a feature request, or want to contribute improvements, please open an issue or submit a pull request.
