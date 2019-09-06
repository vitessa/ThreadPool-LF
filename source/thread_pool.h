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
        _vecThread.emplace_back( CThreadPool::route, this );
    }
    CThreadPool(int thread_num) : _isStop(false)
    {
        for (int i=0; i<thread_num; ++i) {
            _vecThread.emplace_back( CThreadPool::route, this );
        }
    }
    ~CThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _isStop = true;
        }
        _condFollower.notify_all();
        _condLeader.notify_all();
        for (std::thread& t : _vecThread) {
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
            
            this->_queTask.emplace([task](){(*task)();});
        }
        
        this->_condLeader.notify_one();
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
                if (tp->_leaderId == std::thread::id()) { // Leader 
                    tp->_leaderId = std::this_thread::get_id();
                    tp->_condLeader.wait(lock, [tp]{return tp->_isStop || !tp->_queTask.empty();});
                    if (tp->_queTask.empty()) {
                        return;
                    }
                    tp->_leaderId = std::thread::id();
                    task = std::move(tp->_queTask.front());
                    tp->_queTask.pop();
                }
                else { // Follower
                    tp->_condFollower.wait(lock, [tp]{return tp->_isStop || tp->_leaderId == std::thread::id();});
                    if (tp->_isStop && tp->_queTask.empty()) {
                            return;
                    }
                    continue;
                }
            }   // lock_guard end
            
            //Call Follower to become Leader
            tp->_condFollower.notify_one();
            // Worker
            task();
        }
    }
private:
    std::vector<std::thread> _vecThread;             // 线程组
    std::queue< std::function<void()> > _queTask;    // 任务队列
    std::mutex _mtx;                                 // 互斥锁
    bool _isStop;                                    // 是否停止（析构函数用）
    std::condition_variable _condFollower;
    std::condition_variable _condLeader;
    std::thread::id _leaderId;
};

}

#endif  //__VITESSA_THREAD_POOL_H__

