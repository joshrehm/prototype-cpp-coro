#pragma once

#include<condition_variable>
#include<coroutine>
#include<mutex>
#include<queue>

class scheduler 
{
public:
	scheduler();
	scheduler(scheduler&&) = default; 
	scheduler(scheduler const&) = delete;
	~scheduler() = default;

public:
	scheduler& operator=(scheduler&&) = default;
	scheduler& operator=(scheduler const&) = delete;

public:
	void enqueue(std::coroutine_handle<> h);

public:
	void run();
	void stop();

private:
	std::queue<std::coroutine_handle<>> queue_;
	std::condition_variable trigger_;
	std::mutex lock_;
	bool running_;
};

namespace this_scheduler {
	scheduler& get();
}
