#include"prototype_cpp_coro/prototype_cpp_coro.h"
#include"prototype_cpp_coro/scheduler.h"
#include"prototype_cpp_coro/task.h"
#include<cassert>
#include<future>
#include<iostream>
#include<thread>

task<int> coro_main(int argc, char* argv[]);

namespace {
    // TODO: put async_scheduler, set_default_work_scheduler, and begin_async_task in
    //       a header/implementation. But we're just prototyping.
    scheduler* async_scheduler = nullptr;

    void set_default_work_scheduler(scheduler& scheduler) 
    {
        assert(async_scheduler == nullptr);
        async_scheduler = &scheduler;
    }

    // TODO: Make void versions of this
    // TODO: Make auto-deduce return value version of this

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

    template<typename TResult, typename TCallable>
    task<TResult> begin_async_task(TCallable&& invoke)
    {
        co_return co_await begin_scheduled_task<TResult>(*async_scheduler,
            std::forward<TCallable>(invoke));
    }

    task<int> begin_main(scheduler& s, int argc, char* argv[])
    {
        // scheduler was captured by the task promise constructor
        auto result = co_await coro_main(argc, argv);
        s.stop();

        co_return result;
    }
}

int main(int argc, char* argv[])
{
    scheduler worker; 
    set_default_work_scheduler(worker);

    std::promise<void> threads_signal;
    auto threads_ready = threads_signal.get_future();

    auto async = std::thread { [&]() { 
            threads_signal.set_value();
            worker.run(); 
        }
    };
    threads_ready.wait();

    scheduler dispatcher;
    auto main = begin_main(dispatcher, argc, argv);

    dispatcher.run();

    worker.stop();
    async.join();

    // NOTE: exit_code may throw. We don't worry about that, here.
    return main.result();
}

task<int> coro_main(int argc, char* argv[])
{
    std::cout << "Executed `main` on " << std::this_thread::get_id() << ".\n";
    
    // TODO: Auto-deduce return type
    // TODO: Make begin_async_task handle callables that are also coroutines
    auto result = co_await begin_async_task<int>([]() -> int {
            std::cout << "Executed on worker thread " << std::this_thread::get_id() << ".\n";
            return 5;
        });

    std::cout << "Resumed `main` on " << std::this_thread::get_id() 
              << ". Result of task was " << result << ".\n";

    co_return result;
}
