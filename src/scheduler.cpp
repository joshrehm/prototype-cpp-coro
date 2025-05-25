#include"prototype_cpp_coro/scheduler.h"
#include<cassert>
#include<stdexcept>

// Store the executing scheduler for this thread
thread_local scheduler* active_scheduler = nullptr;

namespace this_scheduler {
	// Allows us to get the scheduler currently associated with this thread
	scheduler& get() {
		assert(active_scheduler);
		if (!active_scheduler)
			throw std::runtime_error("No active scheduler on this thread");
		return *active_scheduler;
	}
}

scheduler::scheduler() :
	running_(false)
{
}

// Enqueue a coroutine handle to be resumed
void scheduler::enqueue(std::coroutine_handle<> h)
{
	{
		auto guard = std::unique_lock{ lock_ };
		queue_.push(h);
	}
	trigger_.notify_one();
}

// Shut down our main loop
void scheduler::stop()
{
	{
		auto guard = std::unique_lock{ lock_ };
		if (!running_)
			return;
		running_ = false;
	}
	trigger_.notify_one();
}

void scheduler::run()
{
	std::coroutine_handle<> h;
	std::queue<std::coroutine_handle<>> resumable;

	assert(running_ == false);
	running_ = true;

	// Don't instantiate more than one scheduler per thread.
	assert(active_scheduler == nullptr);
	
	// We defer setting this until a thread calls run in case
	// the main thread creates multiple schedulers given to 
	// other threads.
	active_scheduler = this;

	// We block until we're stopped
	while (running_)
	{
		{
			// Wait until we get a coroutine to resume or we're asked to stop
			auto guard = std::unique_lock{ lock_ };
			trigger_.wait(guard, [&]() {
				return !running_ || !queue_.empty(); });
			
			// If we were asked to stop, destroy our queued coroutines. We don't
			// allow them to run to finish any remaining tasks because they may
			// try to queue more handles, which would cause a deadlock.
			if (!running_)
			{
				while (!queue_.empty())
				{
					queue_.front().destroy();
					queue_.pop();
				}
				continue;
			}
			
			// Since our batch (`resumable`) queue is empty, simply swap it with
			// our current queue_. This effectively clears `queue_` and stores
			// the current batch of handles into `resumable`.
			//
			// Swapping this way allows us to unlock the mutex to unblock any
			// calls to enqueue while we resume our current batch of coroutine
			// handles.
			std::swap(resumable, queue_);
		}

		// Resume every coroutine in our current batch
		while (!resumable.empty())
		{
			h = resumable.front();
			resumable.pop();

			h.resume();
		}
	}
}
