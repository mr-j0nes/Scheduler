# CppScheduler C++ Library ![Test](https://github.com/mr-j0nes/Scheduler/actions/workflows/ci.yml/badge.svg)

CppScheduler is an improved fork of the original [Bosma repository](https://github.com/Bosma/Scheduler), providing a simple and flexible task scheduling framework for C++. The library allows you to schedule tasks to run at specific intervals, specific times, or based on cron expressions. You can also enable or disable tasks dynamically during runtime, as well as removing them via their Id.

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

## Dependencies

The following dependencies are required for CppScheduler:

- [CTPL](https://github.com/vit-vit/CTPL) (C++ Thread Pool Library) [link to CTPL repo]
- [croncpp](https://github.com/mariusbancila/croncpp) (Cron Expression Parser for C++)

Please make sure to initialize the submodules by using the `--recurse-submodules` option when cloning the repository:

```bash
git clone --recurse-submodules https://github.com/mr-j0nes/Scheduler.git
```

## Usage

```cpp
#include "Cppsched/Scheduler.hpp"

// Create a scheduler with a maximum number of threads
Cppsched::Scheduler scheduler(4);

// Schedule a task to run every 5 seconds
scheduler.every("Task1", std::chrono::seconds(5), []() {
    // Your task code here
    // This task will be executed every 5 seconds
});

// Schedule a task to run at a specific time
scheduler.at("Task2", "2023-08-01 12:00:00", []() {
    // Your task code here
    // This task will be executed once at the specified time
});

// Schedule a task to run with a cron expression
scheduler.cron("Task4", "*/5 * * * *", []() {
    // Your task code here
    // This task will be executed every 5 minutes
});

// Schedule a task to run at intervals without concurrency
scheduler.interval("Task5", std::chrono::seconds(10), []() {
    // Your task code here
    // This task will be executed every 10 seconds, and no multiple instances will run concurrently
});

// Enable or disable tasks dynamically during runtime
scheduler.disable_task("Task1");
scheduler.enable_task("Task1");

// Remove tasks
scheduler.remove_task("Task1");
```

**Note**: The difference between `interval` and `every` is that multiple instances of a task scheduled with `interval` will never be run concurrently, ensuring that the task is always completed before the next execution. On the other hand, tasks scheduled with `every` will run at the specified interval regardless of the completion time of the previous instance.

## License

CppScheduler is released under the [MIT License](LICENSE).

## Contributing

Contributions to CppScheduler are welcome! If you find a bug, have a feature request, or want to contribute improvements, please open an issue or submit a pull request.
