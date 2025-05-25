#pragma once

#include"scheduler.h"
#include<chrono>
#include<coroutine>
#include<future>

namespace detail {
	// Base type shared by task<T> and task<void> for controlling how task based
	// coroutines start and end.
	struct promise_type_base
	{
		std::exception_ptr ex_;             // The exception thrown by the coroutine
		std::coroutine_handle<> awaiting_;  // The coroutine we need to resume when
											// we're done
		scheduler* awaiting_scheduler_;     // The scheduler on which the awaiting 
											// coroutine should be resumed
		scheduler* my_scheduler_;			// The scheduler this coroutine will be 
											// resumed on

		promise_type_base() :
			awaiting_scheduler_(nullptr),
			my_scheduler_(nullptr)
		{
		}

		// When our coroutine is initially suspended, we need to capture our
		// scheduler  and queue our execution on that scheduler.
		auto initial_suspend() noexcept
		{
			struct awaiter
			{
				scheduler* my_scheduler;
				
				// We're never ready because we always start suspended.
				bool await_ready() const noexcept
					{ return false; }

				// When we're suspended, queue execution on our scheduler
				// The scheduler will resume our coroutine when it gets to it
				void await_suspend(std::coroutine_handle<> h) noexcept
					{ my_scheduler->enqueue(h); }

				// We don't need to do anything when we're resumed.
				void await_resume() const noexcept
					{ }
			};
			// If the my_scheduler_ hasn't been captured, default to this
			// thread's scheduler.
			if (my_scheduler_ == nullptr)
				my_scheduler_ = &this_scheduler::get();

			return awaiter{ my_scheduler_ };
		}

		// When our coroutine is about to finish, we may need to resume another
		// coroutine that's waiting on us. We handle that here.
		auto final_suspend() const noexcept
		{
			struct awaiter
			{
				scheduler* awaiting_scheduler;
				std::coroutine_handle<> awaiting;

				// We're never ready because we don't know what the awaiting
				// coroutine is going to do.
				bool await_ready() const noexcept
					{ return false; }

				// Queue execution of the awaiting coroutine back on its original
				// scheduler
				void await_suspend(std::coroutine_handle<>) const noexcept
				{
					if (awaiting_scheduler)
						awaiting_scheduler->enqueue(awaiting); 
				}
			
				// Also nothing to do when we resume
				void await_resume() const noexcept
					{ }
			};

			return awaiter { awaiting_scheduler_, awaiting_ };
		}

		// If an exception is thrown by the coroutine, we capture its exception
		// pointer here. This allows us to rethrow when the caller attempts to
		// get the result.
		void unhandled_exception() noexcept
			{ ex_ = std::current_exception(); }
	};
}

// The task<T> class handles the coroutine lifetime as well as defines how the
// coroutine should be started, cleaned up, and how its result should be
// acquired.
//
// We also make our task an awaitable so that it can be co_awaited. 
template<typename T>
class task
{
public:
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

public:
	// The promise type implementation for task<T>
	struct promise_type : public detail::promise_type_base
	{
		std::promise<T> promise;   // Our result setter
		std::future<T> future;     // Our result getter

		promise_type() :
			future(promise.get_future())
		{
		}

		// Specialized constructor that captures the scheduler for any coroutine
		// that specifies one as the first parameter. We assume that that 
		// scheduler is where the coroutine should be resumed.
		template<typename... TIgnoreArgs>
		promise_type(scheduler& scheduler, TIgnoreArgs&&...) :
			promise_type()
		{
			my_scheduler_ = &scheduler;
		}

		// Creates a task<T> for this promise.
		auto get_return_object() noexcept
			{ return task { handle_type::from_promise(*this) }; }

		// Captures the value returned via co_return and stores it
		// so that it can be retrieved by the caller.
		template<typename U>
		void return_value(U&& value)
			{ promise.set_value(std::forward<U>(value)); }
	};


public:
	// Default constructor. Allows us to create a task<T> to assign later.
	task() noexcept :
		handle_(nullptr)
	{
	}

	// Used by promise_type to create a task that's actually managing a
	// coroutine
	task(handle_type h) noexcept :
		handle_(h)
	{
	}

	// Move constructor. The other task<T> no longer manages this
	// coroutine
	task(task&& other) noexcept :
		handle_(other.handle_)
	{
		other.handle_ = nullptr;
	}

	// Tasks are non-copyable
	task(task const& other) = delete;
	
	// When we go, the coroutine frame is coming with us.
	~task() noexcept
	{
		if (handle_)
			handle_.destroy();
	}

public:
	// Move assignment operator. Takes ownership of this coroutine
	// frame.
	task& operator=(task&& other) noexcept
	{
		if (handle_)
			handle_.destroy();
		handle_ = other.handle_;
		other.handle_ = nullptr;
		return *this;
	}

	// Can't copy by assignment
	task& operator=(task const&) = delete;

	// Below is our awaitable implementation for task<T>. These three
	// functions allow task<T> to be co_awaited.
public:
	// If we already have our result, we don't need to resume the coroutine.
	// It's already been finished.
	bool await_ready() noexcept
	{
		// Use wait_for() because future doesn't have a `has_value`
		using namespace std::chrono_literals;
		return handle_.promise().future.wait_for(0s) == std::future_status::ready;
	}

	// When another coroutine awaits this task, we capture that coroutine handle and
	// scheduler. The promise type uses this to resume the other coroutine when the 
	// this coroutine finishes. We capture the scheduler as well so that we ensure 
	// other coroutine resumes on its original execution context.
	// 
	// For example:
	// 
	// task<void> my_task() {         // The 'awaiting' coroutine currently executing
	//     co_await some_async_task() // This task's coroutine. We must resume `awaiting`
	//                                // when we're done
	// }
	//
	void await_suspend(std::coroutine_handle<> awaiting) noexcept
	{
		handle_.promise().awaiting_scheduler_ = &this_scheduler::get();
		handle_.promise().awaiting_ = awaiting; 
	}

	// Return the result of our coroutine to co_await
	T await_resume()
		{ return result(); }

public:
	// Get the result of this coroutine. If the coroutine threw an exception, 
	// we rethrow it here. Otherwise, return the value stored in our promise's
	// future.
	T result()
	{ 
		// TODO: this check on ex_ may not be thread safe if result() is called
		//       by other threads. We handle this with future on value, but not
		//       for the exception. Could just say it's undefined and call it a day.
		//       If we're going to do that, we might as well just store T and
		//       a bool flat for when it's set.
		if (handle_.promise().ex_)
			std::rethrow_exception(handle_.promise().ex_);
		return handle_.promise().future.get(); 
	}

private:
	handle_type handle_;            // Our coroutine handle
};

// This is a specialization for task<T> for type `void`. It behaves exactly the same
// as task<T> above, except it doesn't return a value, so we don't comment it. I'd
// just be rewriting the same comments again.
template<>
class task<void>
{
public:
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

public:
	struct promise_type : public detail::promise_type_base
	{
		std::promise<void> promise;
		std::future<void> future;
		
		promise_type() :
			future(promise.get_future())
		{
		}
		
		template<typename... TIgnoreArgs>
		promise_type(scheduler& scheduler, TIgnoreArgs&&...) :
			promise_type()
		{
			my_scheduler_ = &scheduler;
		}

		auto get_return_object()
			{ return task { handle_type::from_promise(*this) }; }

		void return_void()
			{ promise.set_value(); }
	};

public:
	task() noexcept :
		handle_(nullptr)
	{
	}

	task(handle_type h) noexcept :
		handle_(h)
	{
	}

	task(task&& other) noexcept :
		handle_(other.handle_)
	{
		other.handle_ = nullptr;
	}

	task(task const&) = delete;

	~task() noexcept
	{
		if (handle_)
			handle_.destroy();
	}

public:
	bool await_ready() noexcept
	{
		using namespace std::chrono_literals;
		return handle_.promise().future.wait_for(0s) == std::future_status::ready;
	}

	void await_suspend(std::coroutine_handle<> awaiting) noexcept
	{
		handle_.promise().awaiting_scheduler_ = &this_scheduler::get();
		handle_.promise().awaiting_ = awaiting; 
	}

	void await_resume()
		{ result(); }

	void result()
	{
		if (handle_.promise().ex_)
			std::rethrow_exception(handle_.promise().ex_);
		handle_.promise().future.get();
	}

private:
	handle_type handle_;
};
