#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "Scheduler.hpp"

void message(const std::string &s) { std::cout << s << std::endl; }

int main() {
  // number of tasks that can run simultaneously
  // Note: not the number of tasks that can be added,
  //       but number of tasks that can be run in parallel
  unsigned int max_n_threads = 12;

  // Make a new scheduling object.
  // Note: s cannot be moved or copied
  TaskScheduler::Scheduler s(max_n_threads);

  // every second call message("every second")
  s.every("every", std::chrono::seconds(1), message, "every second");

  // Duplicate task
  try {
    s.every("every", std::chrono::seconds(1), message, "every second");
  } catch (const TaskScheduler::TaskAlreadyExists &e) {
    std::cerr << "ERROR: " << e.what() << "\n";
  }

  // in one minute
  s.in("in", std::chrono::minutes(1) + std::chrono::seconds(2) + std::chrono::milliseconds(500),
       []() { std::cout << "in one minute" << std::endl; });
  // run lambda, then wait a second, run lambda, and so on
  // different from every in that multiple instances of the function will never
  // be run concurrently
  s.interval("interval", std::chrono::seconds(1), []() {
    std::cout << "right away, then once every 6s" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
  });

  // https://en.wikipedia.org/wiki/Cron
  s.cron("cron", "* * * * *",
         []() { std::cout << "top of every minute" << std::endl; });

  // Time formats supported:
  // %Y/%m/%d %H:%M:%S, %Y-%m-%d %H:%M:%S, %H:%M:%S
  // With only a time given, it will run tomorrow if that time has already
  // passed. But with a date given, it will run immediately if that time has
  // already passed.
  s.at("at", "2123-08-02 16:29:18",
       []() { std::cout << "at a specific time." << std::endl; });

  // At now plus 9 seconds
  s.at("at2", std::chrono::system_clock::now() + std::chrono::seconds(9),
       []() { std::cout << "at another specific time." << std::endl; });

  // Wrong date
  try {
  s.at("at3", "223-08-0216:29:18",
       []() { std::cout << "at a specific time." << std::endl; });
  } catch (const TaskScheduler::BadDateFormat &e) {
    std::cerr << "ERROR: " << e.what() << "\n";
  }

  // built-in simple cron calculator, uses local time, see Cron.h
  // expression format:
  // from https://en.wikipedia.org/wiki/Cron#Overview
  //      ┌───────────── minute (0 - 59)
  //      │ ┌───────────── hour (0 - 23)
  //      │ │ ┌───────────── day of month (1 - 31)
  //      │ │ │ ┌───────────── month (1 - 12)
  //      │ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday)
  //      │ │ │ │ │
  //      │ │ │ │ │
  s.cron("cron2", "5 0 * * *", []() {
    std::cout << "every day 5 minutes after midnight" << std::endl;
  });

  // Bad cron expression
  try {
      s.cron("cron3", "blah blah", []() {
        std::cout << "Wrong expression" << std::endl;
      });
  } catch (const TaskScheduler::BadCronExpression &e) {
    std::cerr << "ERROR: " << e.what() << "\n";
  }

  // using https://github.com/staticlibs/ccronexpr
  // Note: uses UTC unless compiled with -DCRON_USE_LOCAL_TIME
  // Note: first field is seconds
  // Supports more advanced expressions:
  // expression           current time           next cron time
  // "*/15 * 1-4 * * *",  "2012-07-01_09:53:50", "2012-07-02_01:00:00"
  // "0 */2 1-4 * * *",   "2012-07-01_09:00:00", "2012-07-02_01:00:00"
  // "0 0 7 ? * MON-FRI", "2009-09-26_00:42:55", "2009-09-28_07:00:00"
  // "0 30 23 30 1/3 ?",  "2011-04-30_23:30:00", "2011-07-30_23:30:00"
  s.ccron("ccron", "*/5 * 0-2 * * *", []() {
    std::cout << "every 5 seconds between 0:00-2:00 UTC" << std::endl;
  });

  // Bad ccron expression
  try {
      s.ccron("ccron2", "blah blah", []() {
        std::cout << "Wrong expression" << std::endl;
      });
  } catch (const TaskScheduler::BadCronExpression &e) {
    std::cerr << "ERROR: " << e.what() << "\n";
  }


  ctpl::thread_pool threads(2);


  // Print all tasks
  threads.push([&s](int){
      while(true)
      {
        for (const auto &task : s.get_tasks_list())
        {
          std::cout << "-> Task Id: <" << task.id << "> 	trigger: <" << task.time_str << ">	enabled: <" << task.enabled << ">\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(15) + std::chrono::milliseconds(500));
      }
      });

  // Disable/Enable/Remove tasks
  threads.push([&s](int){
          std::this_thread::sleep_for(std::chrono::seconds(10));
          s.disable_task("every");
          std::cout << "Disabled: every\n";
          std::this_thread::sleep_for(std::chrono::seconds(10));
          s.enable_task("every");
          std::cout << "Enabled: every\n";
          std::this_thread::sleep_for(std::chrono::seconds(10));
          s.remove_task("every");
          std::cout << "Removed: every\n";
          });

  // destructor of TaskScheduler::Scheduler will cancel all schedules but finish any
  // tasks currently running
  std::this_thread::sleep_for(std::chrono::minutes(10));

  return 0;
}
