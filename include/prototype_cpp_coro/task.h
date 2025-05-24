#pragma once

#include"scheduler.h"
#include<chrono>
#include<coroutine>
#include<future>

namespace detail {
	struct promise_type_base
	{
		std::exception_ptr ex_;
		std::coroutine_handle<> awaiting_;
		scheduler* awaiting_scheduler_;
		scheduler* my_scheduler_;

		promise_type_base() :
			awaiting_scheduler_(nullptr),
			my_scheduler_(nullptr)
		{
		}

		auto initial_suspend() noexcept
		{
			struct awaiter
			{
				scheduler* my_scheduler;
				bool await_ready() const noexcept
					{ return false; }

				void await_suspend(std::coroutine_handle<> h) noexcept
					{ my_scheduler->enqueue(h); }

				void await_resume() const noexcept
					{ }
			};
			if (my_scheduler_ == nullptr)
				my_scheduler_ = &this_scheduler::get();
			return awaiter{ my_scheduler_ };
		}

		auto final_suspend() const noexcept
		{
			struct awaiter
			{
				scheduler* awaiting_scheduler;
				std::coroutine_handle<> awaiting;

				bool await_ready() const noexcept
					{ return false; }

				void await_suspend(std::coroutine_handle<>) const noexcept
				{
					if (awaiting_scheduler)
						awaiting_scheduler->enqueue(awaiting); 
				}
				
				void await_resume() const noexcept
					{ }
			};
			return awaiter { awaiting_scheduler_, awaiting_ };
		}

		void unhandled_exception() noexcept
			{ ex_ = std::current_exception(); }
	};
}

template<typename T>
class task
{
public:
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

public:
	struct promise_type : public detail::promise_type_base
	{
		std::promise<T> promise;
		std::future<T> future;

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

		auto get_return_object() noexcept
			{ return task { handle_type::from_promise(*this) }; }

		template<typename U>
		void return_value(U&& value)
			{ promise.set_value(std::forward<U>(value)); }
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

	task(task const& other) = delete;
	
	~task() noexcept
	{
		if (handle_)
			handle_.destroy();
	}

public:
	task& operator=(task&& other) noexcept
	{
		if (handle_)
			handle_.destroy();
		handle_ = other.handle_;
		other.handle_ = nullptr;
		return *this;
	}

	task& operator=(task const&) = delete;

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

	T await_resume()
		{ return result(); }

public:
	// TODO: Figure out big Ts
	T result()
	{ 
		// TODO: this check on ex_ may not be thread safe if result() is called
		//       by other threads. We handle this with future on value, but not
		//       for the exception. Could just say it's undefined and call it a day.
		if (handle_.promise().ex_)
			std::rethrow_exception(handle_.promise().ex_);
		return handle_.promise().future.get(); 
	}

private:
	handle_type handle_;
	std::exception_ptr exception_;
};

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
