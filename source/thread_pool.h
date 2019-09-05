#ifndef __VITESSA_THREAD_POOL_H__
#define __VITESSA_THREAD_POOL_H__

#include <vector>
#include <queue>
#include <future>
#include <functional>

namespace vitessa
{

class CThreadPool
{
public:
	CThreadPool() : _isStop(false)
	{
		_seqThread.emplace_back( CThreadPool::route, this );
	}
	CThreadPool(int thread_num) : _isStop(false)
	{
		for (int i=0; i<thread_num; ++i) {
			_seqThread.emplace_back( CThreadPool::route, this );
		}
	}
	~CThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(_mtx);
			_isStop = true;
		}
		_cond.notify_all();
		for (std::thread& t : _seqThread) {
			t.join();
		}
	}

	template <class Fn, class... Args>
	std::future<typename std::result_of<Fn(Args...)>::type> spawn(Fn&& fx, Args&&... ax)
	{
		using return_type = typename std::result_of<Fn(Args...)>::type;
		auto task = std::make_shared< std::packaged_task<return_type()> > (
			std::bind(std::forward<Fn>(fx), std::forward<Args>(ax)...) );
		
		std::future<return_type> res = task->get_future();
		
		{
			std::unique_lock<std::mutex> lock(this->_mtx);
			
			if (this->_isStop) {
				throw std::runtime_error("spawn a stoped thread pool.");
			}
			
			this->_seqTask.emplace([task](){(*task)();});
		}
		
		this->_cond.notify_one();
		return res;
	}
	
private:
	// 禁止拷贝
	CThreadPool(const CThreadPool& rhs){};
	// 线程原型
	static void route(CThreadPool* tp)
	{
		for (;;)
		{
			std::function<void()> task;
			{   // lock_guard start
				std::unique_lock<std::mutex> lock(tp->_mtx);
				tp->_cond.wait(lock, [tp]{return tp->_isStop || !tp->_seqTask.empty();});
				if (tp->_seqTask.empty()) {
					return;
				}
				task = std::move(tp->_seqTask.front());
				tp->_seqTask.pop();
			}   // lock_guard end
			task();
		}
	}
private:
	std::vector<std::thread> _seqThread;             // 线程组
	std::queue< std::function<void()> > _seqTask;    // 任务队列
	std::mutex _mtx;                                 // 互斥锁
	std::condition_variable _cond;                   // 条件变量
	bool _isStop;                                    // 是否停止（析构函数用）
};

}

#endif  //__VITESSA_THREAD_POOL_H__

