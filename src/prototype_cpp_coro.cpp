#include"prototype_cpp_coro/prototype_cpp_coro.h"
#include"prototype_cpp_coro/scheduler.h"
#include"prototype_cpp_coro/task.h"
#include<future>
#include<thread>

task<int> coro_main(int arg, char* argv[]);

namespace {
    struct main_task
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        
        struct promise_type
        {
            int exit_code;
            scheduler* sched;
            std::exception_ptr ex;

            promise_type(scheduler& scheduler, int, char*[]) :
                    sched(&scheduler)
                { }

            auto get_return_object()
                { return main_task { handle_type::from_promise(*this) }; }

            auto initial_suspend() const noexcept
            {
                struct awaiter
                {
                    scheduler* sched;
                    bool await_ready() const noexcept
                        { return false; }
                    void await_suspend(std::coroutine_handle<> h)
                        { sched->enqueue(h); }
                    void await_resume()
                        { }
                };
                return awaiter { sched };
            }

            std::suspend_always final_suspend() const noexcept
                { return {}; }

            void return_value(int ec) noexcept
                { exit_code = ec; }
            
            void unhandled_exception()
                { ex = std::current_exception(); }
        };

        handle_type handle;
        main_task(handle_type h) :
            handle(h)
        {
        }

        ~main_task()
        {
            if (handle)
                handle.destroy();
        }

        int exit_code() const noexcept
        {
            if (handle.promise().ex)
                std::rethrow_exception(handle.promise().ex);
            return handle.promise().exit_code;
        }
    };

    main_task begin_main(scheduler& s, int argc, char* argv[])
    {
        auto result = co_await coro_main(argc, argv);
        s.stop();

        co_return result;
    }
}

int main(int argc, char* argv[])
{
    scheduler worker; 
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
    return main.exit_code();
}

task<int> coro_main(int argc, char* argv[])
{
    co_return 0;
}
