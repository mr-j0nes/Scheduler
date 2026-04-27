#pragma once

#include <atomic>
#include <chrono>
#include <iomanip>
#include <map>
#include <vector>
#include <cstring>
#include <memory>
#include <ctime>
#include <utility>
#include <string>

#include "ctpl_stl.h"
#include "croncpp.h"

namespace Cppsched {

    using WallClock = std::chrono::system_clock;
    using MonoClock = std::chrono::steady_clock;
    using Duration = std::chrono::nanoseconds;

    class BadDateFormat : public std::exception {
    public:
        explicit BadDateFormat(std::string msg) : msg_(std::move(msg)) {}

        const char *what() const noexcept override { return (msg_.c_str()); }

    private:
        std::string msg_;
    };

    class TaskAlreadyExists : public std::exception {
    public:
        explicit TaskAlreadyExists(std::string msg) : msg_(std::move(msg)) {}

        const char *what() const noexcept override { return (msg_.c_str()); }

    private:
        std::string msg_;
    };

    class BadCronExpression : public std::exception {
    public:
        explicit BadCronExpression(std::string msg) : msg_(std::move(msg)) {}

        const char *what() const noexcept override { return (msg_.c_str()); }

    private:
        std::string msg_;
    };

    struct TaskReport {
      TaskReport(const std::string &id, const std::string &time_str,
                 const std::string &next_run_str, bool enabled):
        id(id), time_str(time_str), next_run_str(next_run_str), enabled(enabled)
      {}

      std::string id;
      std::string time_str;
      std::string next_run_str;
      bool enabled;
    };

    class Task {
    public:
        explicit Task(const std::string &task_id, const std::string &time_str, std::function<void()> &&f, bool recur = false, bool interval = false, bool enabled  = true) :
                id(task_id), time_str(time_str), f(std::move(f)), recur(recur), interval(interval), enabled(enabled) {}

        virtual MonoClock::time_point get_new_time() const = 0;
        MonoClock::time_point get_sch_time() const { return sch_time; }
        void set_sch_time(MonoClock::time_point t) { sch_time = t; }

        std::string id;                     // Unique ID or user-defined name for the task
        std::string time_str;               // String representation of the time trigger
        std::function<void()> f;

        bool recur;
        bool interval;
        bool enabled;                       // Flag to indicate if the task is enabled
        std::atomic<bool> removed {false};  // Flag to indicate if the task is removed.
        MonoClock::time_point sch_time;     // Scheduled time
    };

    class InTask : public Task {
    public:
        explicit InTask(const std::string &task_id, const std::string &time_str, std::function<void()> &&f) : Task(task_id, time_str, std::move(f)) {}

        // dummy time_point because it's not used
        MonoClock::time_point get_new_time() const override { return MonoClock::time_point(MonoClock::duration(0)); }
    };

    class AtTask : public Task { // Basically same as in
    public:
        explicit AtTask(const std::string &task_id, const std::string &time_str, std::function<void()> &&f) : Task(task_id, time_str, std::move(f)) {}

        // dummy time_point because it's not used
        MonoClock::time_point get_new_time() const override { return MonoClock::time_point(MonoClock::duration(0)); }
    };

    class EveryTask : public Task {
    public:
        EveryTask(const std::string &task_id, const std::string &time_str, MonoClock::duration time, std::function<void()> &&f, bool interval = false) :
                Task(task_id, time_str, std::move(f), true, interval), time(time) {}

        MonoClock::time_point get_new_time() const override {
          return MonoClock::now() + time;
        };
        MonoClock::duration time;
    };


    class CronTask : public Task {
    public:
        CronTask(const std::string &task_id, const std::string &time_str, std::string expression, std::function<void()> &&f) : Task(task_id, time_str, std::move(f), true),
                                                                       exp(std::move(expression)) {}

        MonoClock::time_point get_new_time() const override {
            WallClock::time_point next;
            try
            {
                auto cron = cron::make_cron(exp);

                next = cron::cron_next(cron, WallClock::now());
            }
            catch (cron::bad_cronexpr const & e)
            {
                throw BadCronExpression(std::string(e.what()));
            }

            // convert WallClock -> MonoClock
            auto now_sys  = WallClock::now();
            auto now_mono = MonoClock::now();
            return now_mono + (next - now_sys);
        };

        std::string exp;
    };

    inline bool try_parse(std::tm &tm, const std::string &expression, const std::string &format) {
      std::stringstream ss(expression);
      return !(ss >> std::get_time(&tm, format.c_str())).fail();
    }

    class InterruptableSleep {
        using Clock = std::chrono::steady_clock;

        // InterruptableSleep offers a sleep that can be interrupted by any thread.
        // It can be interrupted multiple times
        // and be interrupted before any sleep is called (the sleep will immediately complete)
        // Has same interface as condition_variables and futures, except with sleep instead of wait.
        // For a given object, sleep can be called on multiple threads safely, but is not recommended as behavior is undefined.

    public:
        InterruptableSleep() : interrupted(false) {
        }

        InterruptableSleep(const InterruptableSleep &) = delete;

        InterruptableSleep(InterruptableSleep &&) noexcept = delete;

        ~InterruptableSleep() noexcept = default;

        InterruptableSleep &operator=(const InterruptableSleep &) noexcept = delete;

        InterruptableSleep &operator=(InterruptableSleep &&) noexcept = delete;

        void sleep_for(Clock::duration duration) {
          std::unique_lock<std::mutex> ul(m);
          cv.wait_for(ul, duration, [this] { return interrupted; });
          interrupted = false;
        }

        void sleep_until(Clock::time_point time) {
          std::unique_lock<std::mutex> ul(m);
          cv.wait_until(ul, time, [this] { return interrupted; });
          interrupted = false;
        }

        void sleep() {
          std::unique_lock<std::mutex> ul(m);
          cv.wait(ul, [this] { return interrupted; });
          interrupted = false;
        }

        void interrupt() {
          std::lock_guard<std::mutex> lg(m);
          interrupted = true;
          cv.notify_one();
        }

    private:
        bool interrupted;
        std::mutex m;
        std::condition_variable cv;
    };

    class ThreadPool {
    public:
        virtual ~ThreadPool() = default;
        virtual void push(std::function<void(int)>&& task) = 0;
        virtual void stop() = 0;
    };

    class CtplThreadPool : public ThreadPool {
    public:
        explicit CtplThreadPool(unsigned int max_n_tasks) : pool(max_n_tasks) {}
        void push(std::function<void(int)>&& task) override {
          pool.push(std::move(task));
        }
        void stop() override {
          pool.stop();
        }
    private:
        ctpl::thread_pool pool;
    };

    class Scheduler {
    public:
        // Constructor with default CTPL thread pool
        explicit Scheduler(unsigned int max_n_tasks = 4) : done(false),
          threads(std::unique_ptr<CtplThreadPool>(new CtplThreadPool(max_n_tasks + 1))) {
          threads->push([this](int) {
            run_watcher_loop();
          });
        }

        // Constructor accepting user-defined thread pool
        explicit Scheduler(std::unique_ptr<ThreadPool> thread_pool)
          : done(false), threads(std::move(thread_pool)) {
          threads->push([this](int) {
            run_watcher_loop();
          });
        }
        Scheduler(const Scheduler &) = delete;

        Scheduler(Scheduler &&) noexcept = delete;

        Scheduler &operator=(const Scheduler &) = delete;

        Scheduler &operator=(Scheduler &&) noexcept = delete;

        ~Scheduler() {
          done = true;
          sleeper.interrupt();
          threads->stop();
        }

        template<typename _Callable, typename... _Args>
        void in(const std::string &task_id, const Duration time, _Callable &&f, _Args &&... args) {
          std::string time_str {"in: " + format_duration(time)};
          std::shared_ptr<Task> t = std::make_shared<InTask>(task_id, time_str,
                  std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...));
          add_task(task_id, MonoClock::now() + time, std::move(t));
        }

        template<typename _Callable, typename... _Args>
        void at(const std::string &task_id, const WallClock::time_point time, _Callable &&f, _Args &&... args) {
          std::string time_str {"at: " + format_time_point("%F %T %z", time)};
          std::shared_ptr<Task> t = std::make_shared<AtTask>(task_id, time_str,
                  std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...));
          auto now_sys  = WallClock::now();
          auto now_mono = MonoClock::now();
          auto mono_tp  = now_mono + (time - now_sys);
          add_task(task_id, mono_tp, std::move(t));
        }

        template<typename _Callable, typename... _Args>
        void at(const std::string &task_id, const std::string &time, _Callable &&f, _Args &&... args) {
          // get current time as a tm object
          auto time_now = WallClock::to_time_t(WallClock::now());
          std::tm tm = *std::localtime(&time_now);

          // our final time as a time_point
          WallClock::time_point tp;

          if (try_parse(tm, time, "%H:%M:%S")) {
            // convert tm back to time_t, then to a time_point and assign to final
            tp = WallClock::from_time_t(std::mktime(&tm));

            // if we've already passed this time, the user will mean next day, so add a day.
            if (WallClock::now() >= tp)
              tp += std::chrono::hours(24);
          } else if (try_parse(tm, time, "%Y-%m-%d %H:%M:%S")) {
            tp = WallClock::from_time_t(std::mktime(&tm));
          } else if (try_parse(tm, time, "%Y/%m/%d %H:%M:%S")) {
            tp = WallClock::from_time_t(std::mktime(&tm));
          } else {
            // could not parse time
            throw BadDateFormat("Cannot parse time string: " + time);
          }

          // std::string time_str {"at: " + time};
          std::string time_str {"at: " + format_time_point("%F %T %z", tp)};
          std::shared_ptr<Task> t = std::make_shared<AtTask>(task_id, time_str,
                  std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...));
          auto now_sys  = WallClock::now();
          auto now_mono = MonoClock::now();
          auto mono_tp  = now_mono + (tp - now_sys);
          add_task(task_id, mono_tp, std::move(t));
        }

        template<typename _Callable, typename... _Args>
        void every(const std::string &task_id, const Duration time, _Callable &&f, _Args &&... args) {
          std::string time_str {"every: " + format_duration(time)};
          std::shared_ptr<Task> t = std::make_shared<EveryTask>(task_id, time_str, time, std::bind(std::forward<_Callable>(f),
                                                                                std::forward<_Args>(args)...));
          auto next_time = t->get_new_time();
          add_task(task_id, next_time, std::move(t));
        }

        template<typename _Callable, typename... _Args>
        void cron(const std::string &task_id, const std::string &expression, _Callable &&f, _Args &&... args) {
          std::string time_str {"cron: " + expression};
          std::shared_ptr<Task> t = std::make_shared<CronTask>(task_id, time_str, expression, std::bind(std::forward<_Callable>(f),
                                                                                      std::forward<_Args>(args)...));
          auto next_time = t->get_new_time();
          add_task(task_id, next_time, std::move(t));
        }

        template<typename _Callable, typename... _Args>
        void interval(const std::string &task_id, const Duration time, _Callable &&f, _Args &&... args) {
          std::string time_str {"interval: " + format_duration(time)};
          std::shared_ptr<Task> t = std::make_shared<EveryTask>(task_id, time_str, time, std::bind(std::forward<_Callable>(f),
                                                                                std::forward<_Args>(args)...), true);
          add_task(task_id, MonoClock::now(), std::move(t));
        }

        // Method to remove a task by ID or name
        bool remove_task(const std::string& task_id)
        {
          std::lock_guard<std::mutex> l(lock);

          // Find the task in the tasks_map
          auto task_iterator = tasks_map.find(task_id);
          if (task_iterator != tasks_map.end()) {
            task_iterator->second->removed = true;
            tasks_map.erase(task_iterator);

            return true;
          }

          return false;
        }

        // Method to disable a task by ID or name
        bool disable_task(const std::string& task_id)
        {
          std::lock_guard<std::mutex> l(lock);

          // Find the task in the tasks_map
          auto task_map_iterator = tasks_map.find(task_id);
          if (task_map_iterator != tasks_map.end()) {
            // Disable the task
            auto &task {task_map_iterator->second};
            task->enabled = false;

            return true;
          }

          return false;
        }

        // Method to enable a task by ID or name
        bool enable_task(const std::string& task_id)
        {
          std::lock_guard<std::mutex> l(lock);

          // Find the task in the tasks_map
          auto task_map_iterator = tasks_map.find(task_id);
          if (task_map_iterator != tasks_map.end()) {
            // Enable the task
            auto &task {task_map_iterator->second};
            task->enabled = true;

            return true;
          }

          return false;
        }

        bool has_task(const std::string& task_id)
        {
          std::lock_guard<std::mutex> l(lock);
          return tasks_map.find(task_id) != tasks_map.end();
        }

        std::vector<TaskReport> get_tasks_list()
        {
          std::vector<TaskReport> v;

          {
            std::lock_guard<std::mutex> l(lock);
            for (auto &map_pair : tasks_map)
            {
              auto &task {map_pair.second};
              auto mono_tp = task->get_sch_time();

              auto now_sys  = WallClock::now();
              auto now_mono = MonoClock::now();
              auto sys_tp   = now_sys + (mono_tp - now_mono);
              v.push_back(TaskReport(task->id, task->time_str, format_time_point("%F %T %z", sys_tp), task->enabled));
            }
          }

          return v;
        }

    private:
        std::atomic<bool> done;

        InterruptableSleep sleeper;

        std::multimap<MonoClock::time_point, std::shared_ptr<Task>> tasks;
        std::map<std::string, std::shared_ptr<Task>> tasks_map;
        std::mutex lock;
        std::unique_ptr<ThreadPool> threads;

        void add_task(const MonoClock::time_point time, std::shared_ptr<Task> t) {
          std::lock_guard<std::mutex> l(lock);
          if (t->removed) return;  // Guard against remove_task() firing between the
                                   // removed check in the lambda and this call.
          const std::string &task_id {t->id};
          t->set_sch_time(time);
          tasks.emplace(time, t);
          tasks_map[task_id] = std::move(t);
          sleeper.interrupt();
        }

        void run_watcher_loop() {
          while (!done) {
            bool has_task{false};
            MonoClock::time_point time_of_first_task;
            {
              std::unique_lock<std::mutex> l(lock);
              if (! tasks.empty()) {
                time_of_first_task = tasks.begin()->first;
                has_task = true;
              }
            }
            // We could have a race condition between has_task and below if(). We
            // could use a condition_variable instead but we'd need to be very careful.
            if (! has_task) {
              sleeper.sleep();
            } else {
              sleeper.sleep_until(time_of_first_task);
            }
            manage_tasks();
          }
        }

        void add_task(const std::string& task_id, const MonoClock::time_point time, std::shared_ptr<Task> t) {
          std::lock_guard<std::mutex> l(lock);
          if (tasks_map.find(task_id) == tasks_map.end()) {
            t->set_sch_time(time);
            tasks.emplace(time, t);
            tasks_map[task_id] = std::move(t);
            sleeper.interrupt();
          } else {
            throw TaskAlreadyExists("Task with id <" + task_id + "> already exists");
          }
        }

        void manage_tasks() {
          std::lock_guard<std::mutex> l(lock);

          auto end_of_tasks_to_run = tasks.upper_bound(MonoClock::now());

          // if there are any tasks to be run and removed
          if (end_of_tasks_to_run != tasks.begin()) {
            // keep track of tasks that will be re-added
            decltype(tasks) recurred_tasks;
            // keep track of tasks that will be removed
            std::vector<std::shared_ptr<Task>> non_recurred_tasks;

            // for all tasks that have been triggered
            for (auto i = tasks.begin(); i != end_of_tasks_to_run; ++i) {
              auto &task = (*i).second;
              if (task->interval) {
                // if it's an interval task, only add the task back after f() is completed
                if (task->enabled && ! task->removed) {
                  // Run
                  threads->push([this, task](int) {
                      task->f();
                      // Check removed AFTER executing, before re-scheduling
                      if (task->removed) {
                          return;
                      }
                      // no risk of race-condition,
                      // add_task() will wait for manage_tasks() to release lock
                      add_task(task->get_new_time(), task);
                      });
                } else {
                  // When removed or disabled, still add to recurred but check removed before re-adding
                  recurred_tasks.emplace(task->get_new_time(), std::move(task));
                }
              } else {
                if (task->enabled && ! task->removed) {
                  threads->push([task](int) {
                      task->f();
                      });
                }

                if (task->recur && ! task->removed) {
                  // calculate time of next run and add the new task to the tasks to be recurred
                  recurred_tasks.emplace(task->get_new_time(), std::move(task));
                } else if (!task->recur) {
                  // save non recurred to remove from tasks_map
                  non_recurred_tasks.push_back(task);
                }
              }
            }

            // remove the completed tasks
            tasks.erase(tasks.begin(), end_of_tasks_to_run);

            // re-add the tasks that are recurring
            for (auto &task_pair : recurred_tasks)
            {
              if (! task_pair.second->removed)
              {
                tasks.emplace(task_pair.first, task_pair.second);
                MonoClock::time_point t {task_pair.first};
                auto &task {task_pair.second};
                task->set_sch_time(t);
                const std::string &task_id {task->id};
                tasks_map[task_id] = task;
              }
            }

            // remove from tasks_map
            for (auto &task : non_recurred_tasks) {
              auto task_map_iterator = tasks_map.find(task->id);
              if (task_map_iterator != tasks_map.end()) {
                tasks_map.erase(task_map_iterator);
              }
            }
          }
        }

        template <typename Duration>
        std::string format_time_point(
            const std::string& fmt,
            const std::chrono::time_point<std::chrono::system_clock, Duration>& tp) const
        {
          auto tp_casted =
              std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp);

          // use tp_casted internally
          return format_time_point_impl(fmt, tp_casted);
        }

        std::string format_time_point_impl(
        const std::string& fmt,
        const std::chrono::system_clock::time_point tp) const
        {
          // Convert to time_t
          std::time_t tt = std::chrono::system_clock::to_time_t(tp);

          // Thread-safe conversion to tm
          std::tm tm{};
#if defined(_WIN32)
          localtime_s(&tm, &tt);   // Windows
#else
          localtime_r(&tt, &tm);   // POSIX
#endif

          // Format using std::put_time
          std::ostringstream oss;
          oss << std::put_time(&tm, fmt.c_str());

          return oss.str();
        }

        inline std::string format_duration(std::chrono::nanoseconds timeunit) const
        {
          std::chrono::nanoseconds ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(timeunit);
          std::ostringstream os;
          bool               foundNonZero = false;
          os.fill('0');
          typedef std::chrono::duration<int, std::ratio<86400 * 365>> years;
          const auto y = std::chrono::duration_cast<years>(ns);
          if (y.count())
          {
            foundNonZero = true;
            os << y.count() << "y";
            ns -= y;
          }
          typedef std::chrono::duration<int, std::ratio<86400>> days;
          const auto d = std::chrono::duration_cast<days>(ns);
          if (d.count())
          {
            if (foundNonZero)
              os << ":";
            foundNonZero = true;
            os << d.count() << "d";
            ns -= d;
          }
          const auto h = std::chrono::duration_cast<std::chrono::hours>(ns);
          if (h.count())
          {
            if (foundNonZero)
              os << ":";
            foundNonZero = true;
            os << h.count() << "h";
            ns -= h;
          }
          const auto m = std::chrono::duration_cast<std::chrono::minutes>(ns);
          if (m.count())
          {
            if (foundNonZero)
              os << ":";
            foundNonZero = true;
            os << m.count() << "m";
            ns -= m;
          }
          const auto s = std::chrono::duration_cast<std::chrono::seconds>(ns);
          if (s.count())
          {
            if (foundNonZero)
              os << ":";
            foundNonZero = true;
            os << s.count() << "s";
            ns -= s;
          }
          const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
          if (ms.count())
          {
            if (foundNonZero)
              os << ":";
            foundNonZero = true;
            os << ms.count() << "ms";
            ns -= ms;
          }
          const auto us = std::chrono::duration_cast<std::chrono::microseconds>(ns);
          if (us.count())
          {
            if (foundNonZero)
              os << ":";
            os << us.count() << "us";
            ns -= us;
          }
          if (ns.count())
          {
            if (foundNonZero)
              os << ":";
            foundNonZero = true;
            os << ns.count() << "ns";
          }
          if (! foundNonZero)
          {
            os << "0s";
          }
          return os.str();
        }
    };

} // namespace Cppsched
