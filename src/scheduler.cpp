#include"prototype_cpp_coro/scheduler.h"
#include<cassert>

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

	while (running_)
	{
		{
			auto guard = std::unique_lock{ lock_ };
			trigger_.wait(guard, [&]() {
				return !running_ || !queue_.empty(); });
			// TODO: May want to drain queued coroutines. For now we just
			//       destroy them
			if (!running_)
				continue;
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
