# Scheduler C++ Library [![Tests](https://github.com/mr-j0nes/Scheduler/actions/workflows/test.yml/badge.svg)](https://github.com/mr-j0nes/Scheduler/actions/workflows/test.yml) [![Release](https://github.com/mr-j0nes/Scheduler/actions/workflows/release.yml/badge.svg)](https://github.com/mr-j0nes/Scheduler/actions/workflows/release.yml)

Scheduler is an improved fork of the original [Bosma repository](https://github.com/Bosma/Scheduler), providing a simple and flexible task scheduling framework for C++. The library allows you to schedule tasks to run at specific intervals, specific times, or based on cron expressions. You can also enable or disable tasks dynamically during runtime, as well as removing them via their Id.

## Differences with the original

- Supported enabling/disabling/removing tasks
- Tasks should be added with a unique Id (task name)
- Added test suite
- Built-in cron was removed in favor of [croncpp](https://github.com/mariusbancila/croncpp) (Cron Expression Parser for C++)
- Namespace changed

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
    // and tasks will run concurrently if previous ones have not finished
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
    // This task will be executed every 5 minutes, 
    // starting at the next minute that is module of 5
});

// Schedule a task to run at intervals without concurrency
scheduler.interval("Task5", std::chrono::seconds(10), []() {
    // Your task code here
    // This task will be executed every 10 seconds, 
    // starting now, and no multiple instances will run concurrently
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

## Contribution Guidelines

We welcome contributions to our project! To maintain a clear and organized development history, please adhere to the Conventional Commits message format when making commits.

### Conventional Commits

Please follow the guidelines from the [Conventional Commits website](https://www.conventionalcommits.org/) when crafting your commit messages. This format helps us generate accurate changelogs and automate the release process based on the types of changes you make.

## Automatic Releases

We've streamlined our release process to be automated, thanks to the Conventional Commits message format. This ensures that our project maintains a clear versioning scheme and changelog, without the need for manual intervention.

### How Automatic Releases Work

When you follow the Conventional Commits message format for your commit messages, our automated release system interprets these messages and determines the appropriate version bump for the project.

- Commits with **fix:** in the message trigger a **patch** version increase.
- Commits with **feat:** in the message trigger a **minor** version increase.
- Commits with a **BREAKING CHANGE:** in the message trigger a **major** version increase.

Here's an example of how it works:
- If you contribute a bug fix, such as `fix: resolve login issue`, it will trigger a patch version increase.
- If you add a new feature, such as `feat: implement user profile customization`, it will trigger a minor version increase.
- If your contribution includes a breaking change, such as `BREAKING CHANGE: update authentication method`, it will trigger a major version increase.

### Benefits of Automated Releases

Automated releases offer several benefits to our development workflow:
- **Consistency:** Every release follows a standardized versioning scheme.
- **Changelog Generation:** Changelogs are automatically generated based on commit messages.
- **Efficiency:** Release management is streamlined, saving time and reducing errors.
- **Transparency:** Contributors can see how their changes affect the versioning process.

By adhering to the Conventional Commits format, you play a crucial role in ensuring that our project's releases are accurate, well-documented, and hassle-free.

Thank you for your contributions and for helping us maintain a smooth and automated release process!

### Warning

We do not support exclamation marks (`!`) after `<type>` for triggering breaking changes.
