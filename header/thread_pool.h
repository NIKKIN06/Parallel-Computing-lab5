#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

class ThreadPool
{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable cv;

    bool stop;

    void worker_routine()
    {
        while (true)
        {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                cv.wait(lock, [this]()
                    {
                        return stop || !tasks.empty();
                    });

                if (stop && tasks.empty())
                {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            task();
        }
    }
public:
    ThreadPool(size_t num_threads = 8) : stop(false)
    {
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers.emplace_back(&ThreadPool::worker_routine, this);
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }

        cv.notify_all();

        for (std::thread& worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    template<class F, class... Args> void add_task(F&& f, Args&&... args)
    {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop)
            {
                throw std::runtime_error("Adding task to ThreadPool has been stopped");
            }

            tasks.push(std::function<void()>(task));
        }

        cv.notify_one();
    }
};