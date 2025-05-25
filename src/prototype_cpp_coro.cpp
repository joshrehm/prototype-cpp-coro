#include"prototype_cpp_coro/prototype_cpp_coro.h"
#include"prototype_cpp_coro/scheduler.h"
#include"prototype_cpp_coro/task.h"
#include<cassert>
#include<future>
#include<iostream>
#include<thread>

// The program entry point. Unlike `main`, it allows co_awiting of coroutines.
task<int> coro_main(int argc, char* argv[]);

namespace {
    // The work thread's scheduler.
    // HACK: This is a prototype just to show proof of concept. This, and related
    //       functions, really belong in another compilation unit.
    scheduler* async_scheduler = nullptr;

    // Set the worker thread's scheduler for use with `begin_scheduled_task`.
    void set_default_work_scheduler(scheduler& scheduler) 
    {
        assert(async_scheduler == nullptr);
        async_scheduler = &scheduler;
    }

    // TODO: Make void versions of this
    // TODO: Make auto-deduce return value version of this
    // TODO: Allow begin_scheduled_task to schedule TCallables that are also
    //       coroutines

    // Executes the TCallable on the specified scheduler and allows the caller to
    // co_await the result.
    template<typename TResult, typename TCallable>
    task<TResult> begin_scheduled_task(scheduler& s, TCallable&& invoke)
    {
        struct awaiter
        {
            scheduler* my_sheduler;
            std::function<TResult()> callable;

            bool await_ready() const noexcept{ return false; };
            void await_suspend(std::coroutine_handle<> h) noexcept
                { my_sheduler->enqueue(h); }
            TResult await_resume()
                { return callable(); }
        };

        co_return co_await awaiter { async_scheduler, std::move(invoke) };
    }

    // Executes the TCallable on the worker thread and allows the caller to
    // co_await the result.
    template<typename TResult, typename TCallable>
    task<TResult> begin_async_task(TCallable&& invoke)
    {
        co_return co_await begin_scheduled_task<TResult>(*async_scheduler,
            std::forward<TCallable>(invoke));
    }

    // Wrapper function that invokes `coro_main`. Its purpose is to capture the
    // scheduler `s` via the `task<T>::promise_type::promise_type(scheduler& ...)`
    // constructor without exposing the scheduler to `coro_main`. It also stops
    // the scheduler after `coro_main` returns, allowing the calling function to
    // continue after `coro_main` ends.
    //
    // This ensures that `begin_coro_main`, and as a result `coro_main` are queued
    // for resumption on the provided scheduler.
    task<int> begin_coro_main(scheduler& s, int argc, char* argv[])
    {
        auto result = co_await coro_main(argc, argv);

        // When `coro_main` returns, we need to stop our scheduler so our
        // program shuts down
        s.stop();

        // Return the result of `coro_main`.
        co_return result;
    }
}

// It's `main`. You know what `main` is.
int main(int argc, char* argv[])
{
    // Creat our worker scheduler
    scheduler worker; 
    set_default_work_scheduler(worker);

    // Set up our worker thread. The promise ensures we don't continue
    // operation until the thread is running. Without it, `coro_main` may
    // return before the worker thread starts, which causes a deadlock
    // when `worker.stop()` is called.
    //
    // TODO: Move this to a thread pool class.
    std::promise<void> threads_signal;
    auto threads_ready = threads_signal.get_future();

    auto async = std::thread { [&]() { 
            threads_signal.set_value();
            worker.run(); 
        }
    };
    threads_ready.wait();

    // Create our main thread scheduler. This scheduler is meant to 
    // coordinate resumption and completion of various coroutines.
    scheduler dispatcher;

    // Queue our `coro_main` for execution
    auto main = begin_coro_main(dispatcher, argc, argv);

    // Run our dispatcher. This will exit when `coro_main` exits.
    dispatcher.run();

    // Stop our worker scheduler
    worker.stop();

    // Await for the worker thread to shut down
    async.join();

    // Our `coro_main` implementation attempts to return 5. Confirm that
    // is what we got.
    auto exit_code = main.result();
    assert(exit_code == 5);

    // Return our `coro_main` result.
    // TODO: This could throw if `coro_main` threw an exception. We 
    //       should catch and log it, but we're prototyping here.

    return exit_code;
}

// Our coroutine supported program entry point.
task<int> coro_main(int argc, char* argv[])
{
    // Log the thread id of the thread we're executing on.
    std::cout << "Executed `main` on " << std::this_thread::get_id() << ".\n";
    
    // Await an async task that should run on our worker thread
    auto result = co_await begin_async_task<int>([]() -> int {
            // Output our thread id. This should be different than the id we logged
            // above
            std::cout << "Executed on worker thread " << std::this_thread::get_id() << ".\n";
            return 5;
        });

    // Log the thread id again. If our `task<>` is properly resuming coroutines on
    // their original schedulers, the thread id logged here will match the one
    // originally logged when `coro_main` started.
    std::cout << "Resumed `main` on " << std::this_thread::get_id() 
              << ". Result of task was " << result << ".\n";

    // Return our async task's result, which should be 5.
    co_return result;
}
