#include "concurrency/ThreadPool.hpp"
// boot engine
ThreadPool::ThreadPool(size_t num_threads) : stop(false) {
    // spawn no. of threads
    for (size_t i = 0; i < num_threads; ++i) {
        // each thread runs lambda function infinitely
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    // queue lock
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // go to sleep until work arrives || server is shutting down.
                    this->condition.wait(lock, [this]{
                        return this->stop || !this->tasks.empty();
                    });
                    // if the server is stopping && the queue is empty...kill the thread.
                    if (this->stop && this->tasks.empty()) return;
                    // next task from queue
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                    
                }

                // network task
                task(); 
            }
        });
    }
}
// shutdown engine
ThreadPool::~ThreadPool() {
    {
        // lock queue to safely flip the kill switch
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    // alarm to all sleeping threads
    condition.notify_all();
    // wait for every single thread to finish its current task and exit safely
    for (std::thread& worker : workers) if (worker.joinable()) worker.join();
}