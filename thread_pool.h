#include <iostream>
#include <queue>
#include <mutex>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>

template<typename T>
class SafeQueue
{
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
	
public:
	SafeQueue() {}
	SafeQueue(SafeQueue&& other) 
	{}
	~SafeQueue() {}
	bool empty()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}
	int size()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.size();
	}
	void enqueue(T& t)
	{
		std::unique_lock<std::mutex>lock(m_mutex);
		m_queue.emplace(t);
	}
	bool dequeue(T& t)
	{
		std::unique_lock<std::mutex>lock(m_mutex);
		if (m_queue.empty())
		{
			return false;
		}
		t = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}
};

class ThreadPool
{
private:
	class ThreadWorker
	{
	private:
		int m_id;
		ThreadPool* m_pool;
		
	public:
		ThreadWorker(ThreadPool* pool, const int id) :m_pool(pool),m_id(id){}

		void operator()()
		{
			std::function<void()>func;
			bool flg = false;
			
			while (true)
			{
				{
					std::unique_lock<std::mutex> lock(m_pool->m_mutex);
					m_pool->m_condition.wait(lock, [this] { return m_pool->m_shutdown || !m_pool->m_queue.empty(); });

					if (m_pool->m_shutdown && m_pool->m_queue.empty()) {
						return; // 线程池关闭且任务队列为空时退出
					}

					flg = m_pool->m_queue.dequeue(func);
				}
				if (flg)
				{
					func(); // 执行任务
				}
			}
		}
		
	};
	bool m_shutdown;
	SafeQueue<std::function<void()>> m_queue;
	std::vector<std::thread> m_threads;
	std::mutex m_mutex;
	std::condition_variable m_condition;

public:
	ThreadPool(const int threads_num = 4):m_threads(std::vector<std::thread>(threads_num)),m_shutdown(false){}
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator = (const ThreadPool&) = delete;
	ThreadPool& operator = (ThreadPool&&) = delete;
	void init()
	{
		for (int i = 0;i<m_threads.size();i++)
		{
			m_threads[i] = std::thread(ThreadWorker(this, i));
		}
	}

	void shutdown()
	{
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_shutdown = true;
		}
		m_condition.notify_all();
		for (std::thread& thread : m_threads)
		{
			if (thread.joinable())
			{
				thread.join();  // 等待所有线程完成
			}
		}
	}
	template<typename F,typename ...Args>
	auto submit(F&& f, Args &&...args) -> std::future<decltype(f(args...))>
	{
		std::function<decltype(f(args...))()>func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
		std::function<void()> wrapper_func = [task_ptr]()
			{
				(*task_ptr)();
			};
		m_queue.enqueue(wrapper_func);
		m_condition.notify_one();
		return task_ptr->get_future();
	}
};
