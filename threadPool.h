#pragma once
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace multi_thread {

template<typename Task>
class ThreadSafeQueue {
public:
	ThreadSafeQueue() : _done(false) {

	}

	// push a Task to queue's back.
	void push_back(const Task& t) {
		std::lock_guard<std::mutex> lock(_mtx);
		_queue.push_back(t);
		_ready.notify_one();
	}

	// push a Task to queue's front.
	void push_front(const Task& t) {
		std::lock_guard<std::mutex> lock(_mtx);
		_queue.push_front(t);
		_ready.notify_one();
	}

	// get a task from queue.
	void getTask(Task& t) {
		std::unique_lock<std::mutex> lock(_mtx);
		// queue is empty() and not done, wait until queue has task.
		if (_queue.empty() && !_done)
			_ready.wait(lock);
		if (_done)
			return;
		// queue has task and not done.
		t = _queue.front();
		_queue.pop_front();
	}

	bool empty()const {
		std::lock_guard<std::mutex> lock(_mtx);
		return _queue.empty();
	}

	int size() const {
		std::lock_guard<std::mutex> lock(_mtx);
		return _queue.size();
	}

	void clean() {
		std::lock_guard<std::mutex> lock(_mtx);
		for (int i = 0; i < _queue.size(); ++i)
			_queue.pop_front();
	}

	void done() {
		_done = true;
		_ready.notify_all();
	}
private:
	// change queue to deque.
	std::deque<Task> _queue;
	mutable std::mutex _mtx;
	std::condition_variable _ready;
	std::atomic_bool _done;
};


class ThreadPool {
public:
	ThreadPool(const int threadNum) : _done(false) {
		for (int i = 0; i < threadNum; ++i) {
			_threads.emplace_back(&ThreadPool::workerThread, this);
		}
	}
	~ThreadPool() {
		_done.store(true);
		_queue.done();
		for (auto& thread : _threads) {
			if (thread.joinable())
				thread.join();
		}
	}

	// push a Task into queue's back or front according priority. 
	void submit(const std::function<void(void)>& t, bool priority = false) {
		if (priority)
			_queue.push_front(t);
		else
			_queue.push_back(t);
	}

	bool isEmpty()const {
		return _queue.empty();
	}
	int taskSize()const {
		return _queue.size();
	}
	void cleanTask() {
		_queue.clean();
	}
private:
	void workerThread() {
		while (true) {
			std::function<void(void)> t;
			//  
			_queue.getTask(t);
			if (_done)
				break;
			if (t)
				t();
		}
	}
private:
	std::vector<std::thread> _threads;
	ThreadSafeQueue<std::function<void(void)>> _queue;
	std::atomic_bool _done;
};

}// namespace