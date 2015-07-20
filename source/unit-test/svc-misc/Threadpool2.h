#ifndef THREAD_POOL2_H
#define THREAD_POOL2_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <concurrency/SpinLock.h>

struct TPWorker {
    TPWorker()
    : worker(&TPWorker::workLoop, this),
    stop(false)
    {
    }

    void finish()
    {
        queue_lock.lock();
        stop = true;
        queue_lock.unlock();

        condition.notify_all();
        worker.join();
    }

    void enqueue(std::function<void()> &t) 
    {
        queue_lock.lock();
        tasks.push(t);
        queue_lock.unlock();

        condition.notify_one();
    }
    void workLoop() {
        std::unique_lock<std::mutex> cond_lock(this->cond_mutex);
        for(;;)
        {
            std::function<void()> task;
            queue_lock.lock();
            while (tasks.size() == 0) {
                queue_lock.unlock();
                this->condition.wait(cond_lock);
                queue_lock.lock();
                if (stop) {
                    queue_lock.unlock();
                    return;
                }
            }
            task = std::move(this->tasks.front());
            this->tasks.pop();
            queue_lock.unlock();

            task();
        }
    }

    std::thread worker;
    // the task queue
    std::queue< std::function<void()> > tasks;
    
    // synchronization
    std::mutex cond_mutex;
    fds::fds_spinlock queue_lock;
    std::condition_variable condition;
    bool stop;
};

struct TP {
    TP(uint32_t sz)
    : workers(sz),
      idx(0)
    {
    }
    ~TP()
    {
        for (auto &w : workers) {
            w.finish();
        }
    }
    void enqueue(std::function<void ()> f) {
        int i = idx++ % workers.size();
        workers[i].enqueue(f);
    }
    std::vector<TPWorker> workers;
    std::atomic<int> idx;
};

#if 0
class ThreadPool2 {
public:
    ThreadPool2(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool2();
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    fds::Spinlock queue_lock;
    std::condition_variable condition;
    bool stop;
};
 
// the constructor just launches some amount of workers
inline ThreadPool2::ThreadPool2(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<fds::Spinlock> lock(this->queue_lock);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool2::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<fds::Spinlock> lock(queue_lock);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool2");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool2::~ThreadPool2()
{
    {
        std::unique_lock<fds::Spinlock> lock(queue_lock);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}
#endif

#endif
