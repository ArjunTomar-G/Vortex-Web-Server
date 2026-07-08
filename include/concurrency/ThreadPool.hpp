#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

class ThreadPool{
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();
    template<class F>
    void enqueue(F&& task){
        {
            // 1 Lock
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // 2 stop adding new tasks if the server is shutting down
            if (stop) throw std::runtime_error("Cannot enqueue on a stopped ThreadPool");

            // 3 lambda in a generic void() function
            tasks.emplace(std::forward<F>(task));
        } //release lock
    
        // 4 wake one thread for job
        condition.notify_one();
    }
};