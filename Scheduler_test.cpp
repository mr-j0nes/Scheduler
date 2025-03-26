#include <gtest/gtest.h>

#include <chrono>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <thread>

#include "Scheduler.hpp"

using Clock = std::chrono::system_clock;

inline std::string format_time_point(const std::string &format, const Clock::time_point date)
{
    char       buffer[80] = "";
    std::time_t date_c = std::chrono::system_clock::to_time_t(date);
    std::tm *date_tm = std::localtime(&date_c);

    if (strftime(buffer, sizeof(buffer), format.c_str(), date_tm) == 0)
    {
        throw std::runtime_error("Error in given format <" + format + ">");
    }

    return std::string(buffer);
}

class SchedulerTest : public testing::Test
{
protected:
    std::string taskId {"testId"};
    Cppsched::Scheduler s;
    Clock::duration d_100ms {std::chrono::milliseconds(100)};
    Clock::duration d_50ms {std::chrono::milliseconds(50)};
    Clock::duration d_5ms {std::chrono::milliseconds(5)};
    Clock::duration time_until_task {d_100ms};
    Clock::duration task_duration {d_100ms};
    std::atomic<int> result {0};
    std::atomic<bool> done {false};
    std::function<void()> f;
    std::function<void()> f_except;

    SchedulerTest() :
        f([this](){std::this_thread::sleep_for(task_duration); done = true; ++result;}),
        f_except([this](){throw std::runtime_error("exception");})
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(SchedulerTest, InTask_get_new_time)
{
    Cppsched::InTask inTask(taskId, "blah", [](){});

    EXPECT_EQ(inTask.get_new_time(), Clock::time_point(Clock::duration(0)));
}

TEST_F(SchedulerTest, AtTask_get_new_time)
{
    Cppsched::AtTask atTask(taskId, "blah", [](){});

    EXPECT_EQ(atTask.get_new_time(), Clock::time_point(Clock::duration(0)));
}

TEST_F(SchedulerTest, EveryTask_get_new_time)
{
    Clock::duration dur = std::chrono::seconds(37);

    Cppsched::EveryTask everyTask(taskId, "blah", dur, [](){});

    auto now {format_time_point("%F %T", everyTask.get_new_time())};
    auto next {format_time_point("%F %T", Clock::now() + dur)};

    EXPECT_EQ(now, next);
}

TEST_F(SchedulerTest, CronTask_get_new_time)
{
    // Every 5 seconds but only if it falls in second 0, 5, 10, 15, ..., etc.
    // So, we need to wait til our current time falls in that way:

    while(stoi(format_time_point("%S", Clock::now())) % 5 != 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    const std::string expression {"*/5 * * * * *"};
    Clock::duration dur = std::chrono::seconds(5);

    Cppsched::CronTask cronTask(taskId, "blah", expression, [](){});

    auto now {format_time_point("%F %T", cronTask.get_new_time())};
    auto next {format_time_point("%F %T", Clock::now() + dur)};

    EXPECT_EQ(now, next);
}

TEST_F(SchedulerTest, try_parse)
{
    auto time_now = Clock::to_time_t(Clock::now());
    std::tm tm = *std::localtime(&time_now);

    EXPECT_FALSE(Cppsched::try_parse(tm, "blah blah", "%H:%M:%S"));
    EXPECT_TRUE (Cppsched::try_parse(tm, "15:35:22", "%H:%M:%S"));
}

TEST_F(SchedulerTest, InterruptableSleep)
{
    ctpl::thread_pool threads(1);
    Cppsched::InterruptableSleep is;
    std::atomic<bool> done;

    // sleep_for
    done = false;
    threads.push([&is, &done](int){
            is.sleep_for(std::chrono::milliseconds(100));
            done = true;
            });

    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    EXPECT_TRUE (done);

    // sleep_until
    done = false;
    threads.push([&is, &done](int){
            is.sleep_until(Clock::now() + std::chrono::milliseconds(100));
            done = true;
            });

    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    EXPECT_TRUE (done);

    // sleep
    done = false;
    threads.push([&is, &done](int){
            is.sleep();
            done = true;
            });

    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(done);
    is.interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    EXPECT_TRUE (done);
}

TEST_F(SchedulerTest, Scheduler_disable_notExist)
{
    EXPECT_FALSE(s.disable_task("Blah blah"));
}

TEST_F(SchedulerTest, Scheduler_enable_notExist)
{
    EXPECT_FALSE(s.enable_task("Blah blah"));
}

TEST_F(SchedulerTest, Scheduler_remove_notExist)
{
    EXPECT_FALSE(s.remove_task("Blah blah"));
}

TEST_F(SchedulerTest, Scheduler_in)
{
    // Handles in right time
    done = false;
    s.in(taskId, time_until_task, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task);
    std::this_thread::sleep_for(task_duration /2);
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(task_duration /2);
    EXPECT_TRUE (done);

    // No exceptions
    done = false;
    s.in(taskId, time_until_task, f_except);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task);
    std::this_thread::sleep_for(task_duration);
    EXPECT_FALSE(done);

    // Disable task
    done = false;
    s.in(taskId, time_until_task, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task /2);
    EXPECT_TRUE(s.disable_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);
    std::this_thread::sleep_for(task_duration);
    EXPECT_FALSE(done);

    // Remove task
    done = false;
    s.in(taskId, time_until_task, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task /2);
    EXPECT_TRUE(s.remove_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);
    std::this_thread::sleep_for(task_duration);
    EXPECT_FALSE(done);
}

TEST_F(SchedulerTest, Scheduler_at)
{
    Clock::time_point time;
    std::string expression;

    // Handles in right time
    time = Clock::now() + time_until_task;
    expression = format_time_point("%F %T", time);
    done = false;
    s.at(taskId, time, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task);
    std::this_thread::sleep_for(task_duration /2);
    EXPECT_FALSE(done);
    std::this_thread::sleep_for(task_duration /2);
    EXPECT_EQ(result, 1);

    // No exceptions
    time = Clock::now() + time_until_task;
    expression = format_time_point("%F %T", time);
    done = false;
    s.at(taskId, time, f_except);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task);
    std::this_thread::sleep_for(task_duration);
    EXPECT_FALSE(done);

    // Disable task
    time = Clock::now() + time_until_task;
    expression = format_time_point("%F %T", time);
    done = false;
    s.at(taskId, time, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task /2);
    EXPECT_TRUE(s.disable_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);
    std::this_thread::sleep_for(task_duration);
    EXPECT_FALSE(done);

    // Remove task
    time = Clock::now() + time_until_task;
    expression = format_time_point("%F %T", time);
    done = false;
    s.at(taskId, time, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(time_until_task /2);
    EXPECT_TRUE(s.remove_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);
    std::this_thread::sleep_for(task_duration);
    EXPECT_FALSE(done);
}

TEST_F(SchedulerTest, Scheduler_at_with_expression)
{
    Clock::time_point time;
    std::string expression;
    std::string expression_now;
    auto one_second {std::chrono::seconds(1)};

    // Expression cannot represent milliseconds, so, we have to deal with minimum 1 second

    // Make sure we start at the beginning of the second
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        const time_t durS = std::chrono::duration_cast<std::chrono::seconds>(Clock::now().time_since_epoch()).count();
        const int64_t durMs = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
        if (static_cast<int>(durMs - durS * 1000) > 950)
            break;
    }

    // Handles in right time
    time = Clock::now() + (one_second * 2);
    expression = format_time_point("%F %T", time);
    expression_now = format_time_point("%F %T", Clock::now());
    done = false;
    s.at(taskId, expression, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(one_second);
    EXPECT_FALSE(done) << expression << " " << expression_now;
    std::this_thread::sleep_for(one_second);
    EXPECT_TRUE(done) << expression << " " << expression_now;

    // No exceptions
    time = Clock::now() + (one_second * 2);
    expression = format_time_point("%F %T", time);
    done = false;
    s.at(taskId, expression, f_except);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(one_second * 2);
    EXPECT_FALSE(done);

    // Disable task
    time = Clock::now() + (one_second * 2);
    expression = format_time_point("%F %T", time);
    done = false;
    s.at(taskId, expression, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(one_second);
    EXPECT_TRUE(s.disable_task(taskId));
    std::this_thread::sleep_for(one_second);
    EXPECT_FALSE(done);

    // Remove task
    time = Clock::now() + (one_second * 2);
    expression = format_time_point("%F %T", time);
    done = false;
    s.at(taskId, expression, f);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(one_second);
    EXPECT_TRUE(s.remove_task(taskId));
    std::this_thread::sleep_for(one_second);
    EXPECT_FALSE(done);
}

TEST_F(SchedulerTest, Scheduler_every_non_concurrency)
{
    //
    // Non-concurrency
    //

    // if time_until_task = 100ms and task_duration = 50ms
    //
    //           Now                 100ms                 200ms                 300ms            (Task triggered)
    // Scheduler |--------------------|---------------------|---------------------|-----------
    // 1st Task  |--------------------|==========|----------|---------------------------------
    // 2nd Task  |------------------------------------------|==========|----------------------
    // 3rd Task  |----------------------------------------------------------------|==========|
    //                                          50ms                  50ms                  50ms  (Task end)

    // Make sure task duration is half time a loop
    task_duration = time_until_task / 2;

    // Handles in right time
    result = 0;
    s.every(taskId, time_until_task, f);
    std::this_thread::sleep_for(d_5ms);                             // Delay a bit
    std::this_thread::sleep_for(time_until_task);                   // Wait til 100ms
                                                                    // Here 1st task launched
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 125ms
    EXPECT_EQ(result, 0);                                           // Result not changed
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 150ms
    EXPECT_EQ(result, 1);                                           // Result changed
    result = 0;
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 200ms
                                                                    // Here 2nd task launched
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 225ms
    EXPECT_EQ(result, 0);                                           // Result not changed
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 250ms
    EXPECT_EQ(result, 1);                                           // Result changed

    // Disable task
    result = 0;
    EXPECT_TRUE(s.disable_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 300ms
                                                                    // Here 3rd task launched but no run
    std::this_thread::sleep_for(task_duration);                     // Wait til 350ms
    EXPECT_EQ(result, 0);                                           // Result should not be changed

    // Enable task
    result = 0;
    EXPECT_TRUE(s.enable_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 400ms
                                                                    // Here 4th task launched
    std::this_thread::sleep_for(task_duration);                     // Wait til 450ms
    EXPECT_EQ(result, 1);                                           // Result is changed

    // Remove task
    result = 0;
    EXPECT_TRUE(s.remove_task(taskId));
    std::this_thread::sleep_for(task_duration);                     // Wait til 500ms
                                                                    // Task is not launched as it's removed
    EXPECT_EQ(result, 0);                                           // Result is not changed

    // No exceptions
    result = 0;
    s.every(taskId, task_duration, f_except);
    std::this_thread::sleep_for(task_duration);
    EXPECT_EQ(result, 0);
}

TEST_F(SchedulerTest, Scheduler_every_with_concurrency)
{
    //
    // With-concurrency
    //

    // if time_until_task = 100ms and task_duration = 150ms
    //
    //           Now      100ms      200ms      300ms     400ms      500ms      600ms    (Task triggered)
    // Scheduler |---------|----------|----------|---------|----------|----------|
    // 1st Task  |---------|================|-------------------------------------
    // 2nd Task  |--------------------|===============|---------------------------
    // 3rd Task  |-------------------------------|===============|----------------

    // Make sure task duration is one and half time a loop
    task_duration = time_until_task + (time_until_task / 2);

    // Handles in right time
    result = 0;
    s.every(taskId, time_until_task, f);
    std::this_thread::sleep_for(d_5ms);                             // Delay a bit

    std::this_thread::sleep_for(time_until_task);                   // Wait til 100ms
                                                                    // Here 1st task launched           +
    std::this_thread::sleep_for(time_until_task);                   // Wait til 200ms                   |
                                                                    // Here 2nd task launched           | +
    EXPECT_EQ(result, 0);                                           // Result not changed               | |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 250ms                   | |
                                                                    // Here 1st task ended              + |
    EXPECT_EQ(result, 1);                                           // Result changed                     |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 300ms                     |
                                                                    // Here 3rd task launched             | +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 350ms                     | |
                                                                    // Here 2nd task ended                + |
    EXPECT_EQ(result, 2);                                           // Result changed                       |
                                                                    //                                      |
    // Disable task                                                 //                                      |
    EXPECT_TRUE(s.disable_task(taskId));                            // DISABLED                             |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 400ms                       |
                                                                    // Here 4th task launched not run         | +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 450ms                       | x
                                                                    // Here 2nd task ended                  + x
    EXPECT_EQ(result, 3);                                           // Result changed                         x
                                                                    //                                        x
    // Enable task                                                  //                                        x
    EXPECT_TRUE(s.enable_task(taskId));                             // ENABLED                                x
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 500ms                         x
                                                                    // Here 5th task launched                 x +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 550ms                         x |
                                                                    // Here 4th task would have ended         + |
    EXPECT_EQ(result, 3);                                           // Result is NOT changed                    |
                                                                    //                                          |
    // Remove task                                                  //                                          |
    EXPECT_TRUE(s.remove_task(taskId));                             // REMOVED                                  |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 600ms                           |
                                                                    // Here 6th task launched not run           | +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 650ms                           | x
                                                                    // Here 5th task ended                      + x
    EXPECT_EQ(result, 4);                                           // Result is changed                          x
                                                                    //                                            x
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 750ms                             x
                                                                    // Here 5th task would have ended             +
    EXPECT_EQ(result, 4);                                           // Result is NOT changed

    // No exceptions
    result = 0;
    s.every(taskId, task_duration, f_except);
    std::this_thread::sleep_for(task_duration);
    EXPECT_EQ(result, 0);
}

TEST_F(SchedulerTest, Scheduler_cron_non_concurrency)
{
    //
    // Non-concurrency
    //

    // if time_until_task = 1s and task_duration = 500ms
    //
    //           Now                  1s                    2s                    3s              (Task triggered)
    // Scheduler |--------------------|---------------------|---------------------|-----------
    // 1st Task  |--------------------|==========|----------|---------------------------------
    // 2nd Task  |------------------------------------------|==========|----------------------
    // 3rd Task  |----------------------------------------------------------------|==========|
    //                                          500ms                 500ms                 500ms  (Task end)

    // Make sure loop is one second
    time_until_task = std::chrono::seconds(1);

    // Make sure task duration is half time a loop
    task_duration = time_until_task / 2;

    // Make sure we start at the beginning of the second
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        const time_t durS = std::chrono::duration_cast<std::chrono::seconds>(Clock::now().time_since_epoch()).count();
        const int64_t durMs = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
        if (static_cast<int>(durMs - durS * 1000) < 50)
            break;
    }

    // Handles in right time
    result = 0;

    s.cron(taskId, "* * * * * *", f);
    std::this_thread::sleep_for(d_5ms);                             // Delay a bit
    std::this_thread::sleep_for(time_until_task);                   // Wait til 1s
                                                                    // Here 1st task launched
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 1.25s
    EXPECT_EQ(result, 0);                                           // Result not changed
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 1.5s
    EXPECT_EQ(result, 1);                                           // Result changed
    result = 0;
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 2s
                                                                    // Here 2nd task launched
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 2.25s
    EXPECT_EQ(result, 0);                                           // Result not changed
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 2.5s
    EXPECT_EQ(result, 1);                                           // Result changed

    // Disable task
    result = 0;
    EXPECT_TRUE(s.disable_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 3s
                                                                    // Here 3rd task launched but no run
    std::this_thread::sleep_for(task_duration);                     // Wait til 3.5s
    EXPECT_EQ(result, 0);                                           // Result should not be changed

    // Enable task
    result = 0;
    EXPECT_TRUE(s.enable_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 4s
                                                                    // Here 4th task launched
    std::this_thread::sleep_for(task_duration);                     // Wait til 4.5s
    EXPECT_EQ(result, 1);                                           // Result is changed

    // Remove task
    result = 0;
    EXPECT_TRUE(s.remove_task(taskId));
    std::this_thread::sleep_for(task_duration);                     // Wait til 5s
                                                                    // Task is not launched as it's removed
    EXPECT_EQ(result, 0);                                           // Result is not changed

    // No exceptions
    result = 0;
    s.cron(taskId, "* * * * * *", f_except);
    std::this_thread::sleep_for(task_duration);
    EXPECT_EQ(result, 0);
}

TEST_F(SchedulerTest, Scheduler_cron_with_concurrency)
{
    //
    // With-concurrency
    //

    // if time_until_task = 1s and task_duration = 500ms
    //
    //           Now       1s         2s         3s        4s         5s         6s      (Task triggered)
    // Scheduler |---------|----------|----------|---------|----------|----------|
    // 1st Task  |---------|================|-------------------------------------
    // 2nd Task  |--------------------|===============|---------------------------
    // 3rd Task  |-------------------------------|===============|----------------

    // Make sure loop is one second
    time_until_task = std::chrono::seconds(1);

    // Make sure task duration is one and half time a loop
    task_duration = time_until_task + (time_until_task / 2);

    // Make sure we start at the beginning of the second
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        const time_t durS = std::chrono::duration_cast<std::chrono::seconds>(Clock::now().time_since_epoch()).count();
        const int64_t durMs = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
        if (static_cast<int>(durMs - durS * 1000) < 50)
            break;
    }

    // Handles in right time
    result = 0;
    s.cron(taskId, "* * * * * *", f);
    std::this_thread::sleep_for(d_5ms);                             // Delay a bit

    std::this_thread::sleep_for(time_until_task);                   // Wait til 1s
                                                                    // Here 1st task launched           +
    std::this_thread::sleep_for(time_until_task);                   // Wait til 2s                      |
                                                                    // Here 2nd task launched           | +
    EXPECT_EQ(result, 0);                                           // Result not changed               | |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 2.5s                    | |
                                                                    // Here 1st task ended              + |
    EXPECT_EQ(result, 1);                                           // Result changed                     |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 3s                        |
                                                                    // Here 3rd task launched             | +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 3.5s                      | |
                                                                    // Here 2nd task ended                + |
    EXPECT_EQ(result, 2);                                           // Result changed                       |
                                                                    //                                      |
    // Disable task                                                 //                                      |
    EXPECT_TRUE(s.disable_task(taskId));                            // DISABLED                             |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 4s                          |
                                                                    // Here 4th task launched not run       | +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 4.5s                        | x
                                                                    // Here 2nd task ended                  + x
    EXPECT_EQ(result, 3);                                           // Result changed                         x
                                                                    //                                        x
    // Enable task                                                  //                                        x
    EXPECT_TRUE(s.enable_task(taskId));                             // ENABLED                                x
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 5s                            x
                                                                    // Here 5th task launched                 x +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 5.5s                          x |
                                                                    // Here 4th task would have ended         + |
    EXPECT_EQ(result, 3);                                           // Result is NOT changed                    |
                                                                    //                                          |
    // Remove task                                                  //                                          |
    EXPECT_TRUE(s.remove_task(taskId));                             // REMOVED                                  |
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 6s                              |
                                                                    // Here 6th task launched not run           | +
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 6.5s                            | x
                                                                    // Here 5th task ended                      + x
    EXPECT_EQ(result, 4);                                           // Result is changed                          x
                                                                    //                                            x
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 7.ts                              x
                                                                    // Here 5th task would have ended             +
    EXPECT_EQ(result, 4);                                           // Result is NOT changed

    // No exceptions
    result = 0;
    s.every(taskId, task_duration, f_except);
    std::this_thread::sleep_for(task_duration);
    EXPECT_EQ(result, 0);
}

TEST_F(SchedulerTest, Scheduler_interval_small_task_dur)
{
    //
    // Small task duraction
    //

    // if time_until_task = 100ms and task_duration = 50ms
    // The first task will start at 100ms, it will last 50ms, i.e. it will end
    // at 150ms and the next task starts 100ms later, i.e.: at 250ms
    //
    //           Now       50ms      100ms     150ms     200ms    250ms      300ms
    // Scheduler |-------------------|-------------------|-------------------|-------------------|
    // 1st Task  |=========|---------|-------------------|-------------------|-------------------|
    // 2nd Task  |-------------------|---------|=========|-------------------|-------------------|
    // 3rd Task  |-------------------|-------------------|-------------------|=========|---------|

    // Make sure task duration is half time of the loop
    task_duration = time_until_task / 2;

    // Handles in right time
    result = 0;
    s.interval(taskId, time_until_task, f);                         // Here 1st task launched           +
    std::this_thread::sleep_for(d_5ms);                             // Delay a bit                      |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 25ms                    |
    EXPECT_EQ(result, 0);                                           // Result not changed               |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 50ms                    +
    EXPECT_EQ(result, 1);                                           // Result changed
    std::this_thread::sleep_for(time_until_task);                   // Wait til 150ms
                                                                    // Here 2nd task launched           +
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 175ms                   |
    EXPECT_EQ(result, 1);                                           // Result not changed               |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 200ms                   +
    EXPECT_EQ(result, 2);                                           // Result changed
    result = 0;
    std::this_thread::sleep_for(time_until_task);                   // Wait til 300ms
                                                                    // Here 3rd task launched           +
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 325ms                   |
    EXPECT_EQ(result, 0);                                           // Result not changed               |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 350ms                   +
    EXPECT_EQ(result, 1);                                           // Result changed

    // Disable task
    result = 0;
    EXPECT_TRUE(s.disable_task(taskId));
    std::this_thread::sleep_for(time_until_task);                   // Wait til 450ms
                                                                    // Here 4th task launched but no run+
                                                                    //                                  |
    std::this_thread::sleep_for(task_duration);                     // Wait til 500ms                   +
    EXPECT_EQ(result, 0);                                           // Result should not be changed

    // Enable task
    result = 0;
    EXPECT_TRUE(s.enable_task(taskId));
    std::this_thread::sleep_for(time_until_task /2);                // Wait til 550ms (only 50ms cause
                                                                    // prev task didn't run)
                                                                    // Here 5th task launched           +
                                                                    //                                  |
    std::this_thread::sleep_for(task_duration);                     // Wait til 600ms                   +
    EXPECT_EQ(result, 1);                                           // Result is changed

    // Remove task
    result = 0;
    EXPECT_TRUE(s.remove_task(taskId));
    std::this_thread::sleep_for(time_until_task);                   // Wait til 750ms
                                                                    // Here 6th task launched not run   +
                                                                    //                                  |
    std::this_thread::sleep_for(task_duration);                     // Wait til 800ms                   +
    EXPECT_EQ(result, 0);                                           // Result is NOT changed

    // No exceptions
    result = 0;
    s.interval(taskId, task_duration, f_except);
    std::this_thread::sleep_for(d_5ms);
    std::this_thread::sleep_for(task_duration);
    EXPECT_EQ(result, 0);
}

TEST_F(SchedulerTest, Scheduler_interval_long_task_dur)
{
    //
    // Long task duration
    //

    // if time_until_task = 100ms and task_duration = 150ms
    // So we start our task as 100ms, after finished at 250ms it is scheduled
    // to run again after 100ms, i.e.: at 350ms
    //
    //           Now      100ms      200ms      300ms     400ms      500ms      600ms
    // Scheduler |---------|---------|---------|---------|---------|---------|---------|
    // 1st Task  |==============|------------------------------------------------------|
    // 2nd Task  |------------------------|==============|-----------------------------|
    // 2nd Task  |-------------------------------------------------|==============|----|

    // Make sure task duration is one and half time a loop
    task_duration = time_until_task + (time_until_task / 2);

    // Handles in right time
    result = 0;
    s.interval(taskId, time_until_task, f);                         // Here 1st task launched           +
    std::this_thread::sleep_for(d_5ms);                             // Delay a bit                      |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 75ms                    |
    EXPECT_EQ(result, 0);                                           // Result not changed               |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 150ms                   |
                                                                    // Here 1st task ended              +
    EXPECT_EQ(result, 1);                                           // Result changed

    std::this_thread::sleep_for(time_until_task);                   // Wait til 250ms
                                                                    // Here 2nd task launched           +
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 325ms                   |
    EXPECT_EQ(result, 1);                                           // Result not changed               |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 400ms                   |
                                                                    // Here 2nd task ended              +
    EXPECT_EQ(result, 2);                                           // Result changed
    std::this_thread::sleep_for(time_until_task);                   // Wait til 500ms
                                                                    // Here 3rd task launched           +
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 575ms                   |
    EXPECT_EQ(result, 2);                                           // Result not changed               |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 650ms                   |
                                                                    // Here 3rd task ended              +
    EXPECT_EQ(result, 3);                                           // Result changed

    // Disable task                                                 //
    EXPECT_TRUE(s.disable_task(taskId));                            // DISABLED
    std::this_thread::sleep_for(time_until_task);                   // Wait til 750ms
                                                                    // Here 4th task launched no run    +
    std::this_thread::sleep_for(task_duration);                     // Wait til 900ms                   x
    EXPECT_EQ(result, 3);                                           // Result is NOT changed            x
                                                                    // Here 4th task should have neded  +
    // Enable task                                                  //
    EXPECT_TRUE(s.enable_task(taskId));                             // ENABLED
    std::this_thread::sleep_for(time_until_task);                   // Wait til 1000ms
                                                                    // Here 5th task launched           +
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 1075ms                  |
    EXPECT_EQ(result, 3);                                           // Result is NOT changed            |
    std::this_thread::sleep_for(task_duration /2);                  // Wait til 1150ms                  |
                                                                    // Here 5th task ended              +
    EXPECT_EQ(result, 4);                                           // Result is changed
                                                                    //
    // Remove task                                                  //
    EXPECT_TRUE(s.remove_task(taskId));                             // REMOVED
    std::this_thread::sleep_for(time_until_task);                   // Wait til 1250ms
                                                                    // Here 6th task launched no run    +
    std::this_thread::sleep_for(task_duration);                     // Wait til 1400ms                  x
    EXPECT_EQ(result, 4);                                           // Result is NOT changed            x
                                                                    // Here 6th task should have ended  +


    // No exceptions
    result = 0;
    s.interval(taskId, task_duration, f_except);
    std::this_thread::sleep_for(task_duration);
    EXPECT_EQ(result, 0);
}

TEST_F(SchedulerTest, Scheduler_multithreading)
{
    ctpl::thread_pool threads(4);

    s.every(taskId, time_until_task, f);

    auto a = threads.push([this](int){
            s.disable_task(taskId);
            });

    auto b = threads.push([this](int){
            s.enable_task(taskId);
            });

    auto c = threads.push([this](int){
            s.disable_task(taskId);
            });

    auto d = threads.push([this](int){
            s.enable_task(taskId);
            });

    auto e = threads.push([this](int){
            s.remove_task(taskId);
            });

    EXPECT_NO_THROW(a.get());
    EXPECT_NO_THROW(b.get());
    EXPECT_NO_THROW(c.get());
    EXPECT_NO_THROW(d.get());
    EXPECT_NO_THROW(e.get());
}

TEST_F(SchedulerTest, Scheduler_get_tasks_list)
{
    std::vector<Cppsched::TaskReport> task_report;
    std::vector<Cppsched::TaskReport>::iterator task_report_it;
    Clock::time_point time;
    time = Clock::now() + std::chrono::seconds(10);

    s.every("every1", time_until_task, f);
    s.interval("interval1", time_until_task, f);
    s.at("at1", time, f);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_NO_THROW(task_report = s.get_tasks_list());

    task_report_it = task_report.begin();

    ASSERT_NE(task_report_it, task_report.end());
    EXPECT_EQ(task_report_it->id, "at1");
    EXPECT_EQ(task_report_it->enabled, true);

    ++ task_report_it;

    ASSERT_NE(task_report_it, task_report.end());
    EXPECT_EQ(task_report_it->id, "every1");
    EXPECT_EQ(task_report_it->enabled, true);

    ++ task_report_it;

    ASSERT_NE(task_report_it, task_report.end());
    EXPECT_EQ(task_report_it->id, "interval1");
    EXPECT_EQ(task_report_it->enabled, true);
}
