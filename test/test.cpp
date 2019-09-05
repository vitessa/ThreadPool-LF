
#include "../source/thread_pool.h"
#include <iostream>

vitessa::CThreadPool tp;

static std::mutex mtx;

void hello(int n)
{
    std::thread::id tid = std::this_thread::get_id();
    for (int i=0; i<n; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << tid << " Hello, World!" << std::endl;
    }
}

int add(int lhs, int rhs)
{
    return lhs + rhs;
}

int main(void)
{
    for (int i=0; i<6000; ++i) {
        tp.spawn(hello, 5);
        std::future<int> res = tp.spawn(add, 5, 5);
    }
    return 0;
}

