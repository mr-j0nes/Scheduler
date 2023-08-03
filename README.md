# TaskScheduler C++ Library

TaskScheduler is an improved fork of the original [Bosma repository](https://github.com/Bosma/Scheduler), providing a simple and flexible task scheduling framework for C++. The library allows you to schedule tasks to run at specific intervals, specific times, or based on cron expressions. You can also enable or disable tasks dynamically during runtime.

## Features

- Schedule tasks to run at specific intervals or times.
- Support for cron-like expressions for task scheduling.
- Enable or disable tasks during runtime.
- Thread-safe task scheduling and management.
- Lightweight and easy to integrate into your C++ projects.

## Dependencies

The following dependencies are required for TaskScheduler:

- CTPL (C++ Thread Pool Library) [link to CTPL repo]
- ccronexpr (Cron Expression Parser for C++) [link to ccronexpr repo]

Please make sure to initialize the submodules by using the `--recurse-submodules` option when cloning the repository:

```bash
git clone --recurse-submodules https://github.com/mr-j0nes/Scheduler.git
```

## Usage

```cpp
#include "TaskScheduler/Scheduler.hpp"

// Create a scheduler with a maximum number of threads
TaskScheduler::Scheduler scheduler(4);

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

// Schedule a task based on a cron expression
scheduler.cron("Task3", "0 0 * * *", []() {
    // Your task code here
    // This task will be executed every day at midnight
});

// Schedule a task to run with a ccron expression
scheduler.ccron("Task4", "*/5 * * * *", []() {
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
```

**Note**: The difference between `interval` and `every` is that multiple instances of a task scheduled with `interval` will never be run concurrently, ensuring that the task is always completed before the next execution. On the other hand, tasks scheduled with `every` will run at the specified interval regardless of the completion time of the previous instance.

## License

TaskScheduler is released under the [MIT License](LICENSE).

## Contributing

Contributions to TaskScheduler are welcome! If you find a bug, have a feature request, or want to contribute improvements, please open an issue or submit a pull request.
