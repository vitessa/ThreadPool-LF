#pragma once

#include <vector>
#include <queue>
#include <future>

/*
 * @class 线程池
 *
 * @constructor CThreadPool()   创建一个只有一个线程的线程池
 * @constructor CThreadPool(n)  创建一个有n个线程的线程池
 * @destructor  ~CThreadPool()  销毁线程池，会阻塞直到所有任务完成
 *
 * @THREAD_POOL_SPAWN   向线程池中添加任务，线程安全
 *                      \tp  线程池
 *                      \fn  要执行的函数
 *                      \..  函数的参数
 *                      \rt  返回future<return_type>
 */

#define THREAD_POOL_SPAWN(tp, fn, ...) CThreadPool::spawn(tp,std::bind(fn, ##__VA_ARGS__))

class CThreadPool
{
public:
    CThreadPool() : _isStop(false)
    {
        _vecThread.emplace_back(CThreadPool::route, this);
    }

    CThreadPool(int thread_num) : _isStop(false)
    {
        for (int i=0; i<thread_num; ++i) {
            _vecThread.emplace_back(CThreadPool::route, this);
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

    template<typename Fn>
    static std::future<typename std::result_of<Fn()>::type> spawn(CThreadPool& tp, Fn func)
    {
        typedef std::result_of<Fn()>::type rt_type;

        auto task = std::make_shared<std::packaged_task<rt_type()>>(std::forward<Fn>(func));
        std::future<rt_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(tp._mtx);
            if (tp._isStop) {
                throw std::runtime_error("spawn a stoped thread pool.");
            }
            tp._queTask.emplace([task](){(*task)();});
        }

        tp._condLeader.notify_one();
        return res;
    }

private:
    CThreadPool(const CThreadPool&){}

    static void route(CThreadPool* tp)
    {
        std::function<void()> task;
        for (;;)
        {
            {   // lock_guard start
                std::unique_lock<std::mutex> lock(tp->_mtx);
                if (tp->_leaderId == std::thread::id()) {
                    // Leader
                    tp->_leaderId = std::this_thread::get_id();
                    tp->_condLeader.wait(lock, [tp]{return tp->_isStop || !tp->_queTask.empty();});
                    if (tp->_queTask.empty()) { return; }
                    tp->_leaderId = std::thread::id();
                    task = std::move(tp->_queTask.front());
                    tp->_queTask.pop();
                }
                else {
                    // Follower
                    tp->_condFollower.wait(lock, [tp]{return tp->_isStop || tp->_leaderId==std::thread::id();});
                    if (tp->_isStop && tp->_queTask.empty()) {return;}
                    continue;
                }
            }   // lock_guard end

            // Call Follower to become Leader
            tp->_condFollower.notify_one();
            // Worker
            task();
        }
    }

private:
    std::vector<std::thread> _vecThread;        // 线程组
    std::queue<std::function<void()>> _queTask; // 任务队列
    std::mutex _mtx;                            // 互斥锁
    bool _isStop;                               // 是否停止（析构函数用）
    std::condition_variable _condFollower;
    std::condition_variable _condLeader;
    std::thread::id _leaderId;
};
