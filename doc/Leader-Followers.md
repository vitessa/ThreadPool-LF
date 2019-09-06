
### 8.5.7 如何确定提拔线程

以下三种逻辑可以用来确定追随者如何被提拔为领导者：

 - 后进先出(LIFO)
 
在多数场景中，线程都是对等的，此时线程的提拔顺序可以是任意的。在这种情况下，可以使用后进先出的提拔协议，既是俗话说的“新来新猪肉”。此协议通过最大限度的降低线程的休眠时间，来提高线程对CPU缓存（cache）的亲和力（affinity）。此外，需要额外使用其他方法来为线程排序，例如：堆栈（stack），而不能仅仅使用普通的系统同步手段，例如：信号量（semaphore）。

 - 优先级

在一些对实时性要求很高的应用中，线程往往运行在不同的优先级下。此时，需要按线程优先级来提拔新的领导者。这样也需要额外的数据结构来管理线程的优先级，例如：堆（heap）。假如不使用按优先级的提拔协议，那么就会在优先级反转上带来其他麻烦。


 - 系统定义

这种方法就是直接使用系统的同步工具，例如：信号量（semaphore）、条件变量（condition variable），这写同步工具都是由操作系统定义的顺序执行的。这种方法优点是，能有有效的对应系统的同步手段。

下面为这种方法提供一个示例：

```c++
int LF_Thread_Pool::promote_new_leader(void)
{
    // Use Scoped Locing idion to acquire mutex
    // automatically in the constructor.
    std::unique_lock<std::mutex> lock(mutex_);

    if (leader_thread_ != std::this_thread::get_id()) {
        // Error, only the leader thread can call this.
        return -1;
    }

    // Indicate that we're no longer the leader
    // and notify a <join> method to promote
    // the next follower.
    leader_thread_ = std::thread::id();
    followers_condition_.notify();

    // Release mutex automatically in destructor.
}
```
