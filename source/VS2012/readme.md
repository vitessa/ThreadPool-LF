# VS2012版本  
VS2012只支持部分C++11。  
经过实测VS2012不支持变参模板函数，所以写了一个宏函数版本给VS2012。

# 实例代码
```cpp
#include "thread_pool.h"
#include <iostream>

using namespace std;

// 测试函数
#include "thread_pool.h"
#include <iostream>

using namespace std;

// 测试函数
void printMsg(const string& msg)
{
    cout << msg.c_str() << endl;
}

// 测试函数
int add(int a, int b)
{
    return a + b;
}

int main()
{
    vitessa::CThreadPool tp;

    // void形函数可以忽略返回值
    THREAD_POOL_SPAWN(tp, printMsg, "example:");

    // 其他函数可以通过get()阻塞返回
    auto ret = THREAD_POOL_SPAWN(tp, add, 1, 2);
    std::cout << "1 + 2 = " << ret.get() << std::endl;

    return 0;
}

```
