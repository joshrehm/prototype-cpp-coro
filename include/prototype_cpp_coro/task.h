#pragma once

#include<coroutine>

namespace detail {
	struct promise_type_base
	{
		std::exception_ptr ex;
		std::coroutine_handle<> awaiting_;

		std::suspend_never initial_suspend() const noexcept
			{ return {}; }

		auto final_suspend() const noexcept
		{ 
			struct awaiter
			{
				std::coroutine_handle<> awaiting;
				awaiter(std::coroutine_handle<> awaiting) :
					awaiting(awaiting)
				{
				}
				bool await_ready() const noexcept
					{ return false; }
				
				void await_resume() const noexcept
					{ }

				auto await_suspend(std::coroutine_handle<>) const noexcept
					-> std::coroutine_handle<>
				{
					if (!awaiting)
						return std::noop_coroutine();
					return awaiting;
				}
			};
			return awaiter{ awaiting_ };
		}

		void unhandled_exception() noexcept
			{ ex = std::current_exception(); }
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
		T result;

		auto get_return_object() noexcept
			{ return task { handle_type::from_promise(*this) }; }

		// TODO: Figure out big Ts since we're passing by value
		// TODO: Set noexcept if copying/moving T is noexcept
		void return_value(T value)
			{ result = value; }
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
		{ return handle_.done(); }

	void await_suspend(std::coroutine_handle<> awaiting) noexcept
		{ handle_.promise().awaiting_ = awaiting; }

	T await_resume()
	{
		if (handle_.promise().ex)
			std::rethrow_exception(handle_.promise().ex); 
		return handle_.promise().result; 
	}

public:
	// TODO: Figure out big Ts
	T result()
		{ return handle_.promise().result; }

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
		auto get_return_object()
			{ return task { handle_type::from_promise(*this) }; }

		void return_void() const noexcept {}
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
		{ return handle_.done(); }

	void await_suspend(std::coroutine_handle<> awaiting) noexcept
		{ handle_.promise().awaiting_ = awaiting; }

	void await_resume()
	{ 
		if (handle_.promise().ex)
			std::rethrow_exception(handle_.promise().ex);
	}

private:
	handle_type handle_;
	std::exception_ptr exception_;
};
