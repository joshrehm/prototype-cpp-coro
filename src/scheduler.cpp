#include"prototype_cpp_coro/scheduler.h"
#include<cassert>
#include<stdexcept>

thread_local scheduler* active_scheduler = nullptr;

namespace this_scheduler {
	scheduler& get() {
		assert(active_scheduler);
		if (!active_scheduler)
			throw std::runtime_error("No active scheduler on this thread");
		return *active_scheduler;
	}
}

scheduler::scheduler() :
	running_(true)
{
}

void scheduler::enqueue(std::coroutine_handle<> h)
{
	{
		auto guard = std::unique_lock{ lock_ };
		queue_.push(h);
	}
	trigger_.notify_one();
}

void scheduler::stop()
{
	{
		auto guard = std::unique_lock{ lock_ };
		running_ = false;
	}
	trigger_.notify_one();
}

void scheduler::run()
{
	std::coroutine_handle<> h;
	std::queue<std::coroutine_handle<>> resumable;

	// Don't instantiate more than one scheduler per thread.
	assert(active_scheduler == nullptr);

	// We defer setting this until a thread calls run in case
	// the main thread creates multiple schedulers given to 
	// other threads.
	active_scheduler = this;

	while (running_)
	{
		{
			auto guard = std::unique_lock{ lock_ };
			trigger_.wait(guard, [&]() {
				return !running_ || !queue_.empty(); });
			
			if (!running_)
			{
				while (!queue_.empty())
				{
					queue_.front().destroy();
					queue_.pop();
				}
				continue;
			}
			
			std::swap(resumable, queue_);
		}

		while (!resumable.empty())
		{
			h = resumable.front();
			resumable.pop();

			h.resume();
		}
	}
}
