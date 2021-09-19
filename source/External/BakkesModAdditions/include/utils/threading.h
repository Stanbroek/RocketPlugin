#pragma once
#include <list>
#include <mutex>
#include <future>
#include <thread>
#include <vector>
#include <cassert>
#include <functional>

#ifndef FMT_HEADER_ONLY
    #define FMT_HEADER_ONLY
#endif
#include "fmt/format.h"
#include "fmt/ostream.h"

extern std::thread::id GameThreadId;
extern std::thread::id RenderThreadId;


template<class... Args>
static inline std::thread save_thread(const std::string& comment, const std::function<void()>& lambda)
{
    return std::thread([=]() -> void {
        try {
            return lambda();
        }
        catch (...) {
            BM_CRITICAL_LOG("thread #{:s} {:s} crashed", std::this_thread::get_id(), comment);
        }
    });
}


template<class... Args>
static inline std::thread save_thread(const std::string& comment, void(*func)(Args...), const Args&&... args)
{
    return std::thread([=]() -> void {
        try {
            return func(std::forward<Args>(args)...);
        }
        catch (...) {
            BM_CRITICAL_LOG("thread #{:s} {:s} crashed", std::this_thread::get_id(), comment);
        }
    });
}


template<class Cls, class... Args>
static inline std::thread save_thread(const std::string& comment, void(Cls::*func)(Args...), Cls* cls, const Args&&... args)
{
    return std::thread([=]() -> void {
        try {
            return (cls->*func)(std::forward<Args>(args)...);
        }
        catch (...) {
            BM_CRITICAL_LOG("thread #{:s} {:s} crashed", std::this_thread::get_id(), comment);
        }
    });
}


template<typename Ret>
static inline std::future<Ret> save_promise(const std::string& comment, const std::function<Ret()>& lambda)
{
    return std::async([=]() -> Ret {
        try {
            return lambda();
        }
        catch (...) {
            BM_CRITICAL_LOG("thread #{:s} {:s} crashed", std::this_thread::get_id(), comment);
        }
        return Ret{};
    });
}


template<typename Ret, class... Args>
static inline std::future<Ret> save_promise(const std::string& comment, Ret(*func)(Args...), const Args&&... args)
{
    return std::async([=]() -> Ret {
        try {
            return func(std::forward<Args>(args)...);
        }
        catch (...) {
            BM_CRITICAL_LOG("thread #{:s} {:s} crashed", std::this_thread::get_id(), comment);
        }
        return Ret{};
    });
}


template<typename Ret, class Cls, class... Args>
static inline std::future<Ret> save_promise(const std::string& comment, Ret(Cls::* func)(Args...), Cls* cls, const Args&&... args)
{
    return std::async([=]() -> Ret {
        try {
            return (cls->*func)(std::forward<Args>(args)...);
        }
        catch (...) {
            BM_CRITICAL_LOG("thread #{:s} {:s} crashed", std::this_thread::get_id(), comment);
        }
        return Ret{};
    });
}


static inline void set_game_thread()
{
    GameThreadId = std::this_thread::get_id();
}


static inline void set_game_thread_once()
{
    if (GameThreadId == std::thread::id()) {
        set_game_thread();
    }
}


static inline bool is_game_thread()
{
    return std::this_thread::get_id() == GameThreadId;
}


static inline void assert_game_thread([[maybe_unused]] const wchar_t* message, [[maybe_unused]] const wchar_t* file = L"", [[maybe_unused]] const unsigned int line = 0)
{
    if (!is_game_thread()) {
#ifndef NDEBUG
        _wassert(message, file, line);
#endif
    }
}


static inline void set_render_thread()
{
    RenderThreadId = std::this_thread::get_id();
}


static inline void set_render_thread_once()
{
    if (RenderThreadId == std::thread::id()) {
        set_render_thread();
    }
}


static inline bool is_render_thread()
{
    return std::this_thread::get_id() == RenderThreadId;
}


static inline void assert_render_thread([[maybe_unused]] const wchar_t* message, [[maybe_unused]] const wchar_t* file = L"", [[maybe_unused]] const unsigned int line = 0)
{
    if (!is_render_thread()) {
#ifndef NDEBUG
        _wassert(message, file, line);
#endif
    }
}


struct catch_exceptions_t {
    explicit catch_exceptions_t() = default;
};

inline constexpr catch_exceptions_t catch_exceptions{};


/// <summary>Threaded class that queues <see cref="job_t"/>'s to be executed.</summary>
/// <remarks> From http://www.cplusplus.com/forum/general/87747/ </remarks>
class JobQueue
{
public:
    typedef std::function<void()> job_t;

    /// <summary>Creates a thread to queue to call jobs on.</summary>
    JobQueue()
    {
        thread = std::thread(&JobQueue::entry, this);
    }
    
    /// <summary>Creates a save thread to queue to call jobs on.</summary>
    JobQueue(catch_exceptions_t)
    {
        thread = save_thread("JobQueue", &JobQueue::entry, this);
    }

    /// <summary>Waits for the last job to finish and removes queue.</summary>
    ~JobQueue()
    {
        // Unlocking is done before notifying, to avoid 
        // waking up the waiting thread only to block again.
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            wantExit = true;
        }
        queuePending.notify_one();
        thread.join();
    }

    JobQueue(const JobQueue&) = delete;
    JobQueue(JobQueue&&) = delete;
    JobQueue& operator=(const JobQueue&) = delete;
    JobQueue& operator=(JobQueue&&) = delete;

    /// <summary>Adds job_t's to the job queue to be execute on a separate thread.</summary>
    /// <param name="job">function to be executed</param>
    void addJob(const job_t& job)
    {
        // Unlocking is done before notifying, to avoid 
        // waking up the waiting thread only to block again.
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            jobQueue.push_back(job);
        }
        queuePending.notify_one();
    }

    /// <summary>Checks if the thread is busy.</summary>
    /// <returns>Bool with if the thread is busy</returns>
    bool isBusy() const
    {
        return !jobQueue.empty() || isExecuting;
    }

private:
    /// <summary>Main loop of the thread.</summary>
    void entry()
    {
        job_t job;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                isExecuting = false;
                queuePending.wait(lock, [this]() {
                    return wantExit || !jobQueue.empty();
                });

                if (wantExit && jobQueue.empty()) {
                    return;
                }

                isExecuting = true;
                job = jobQueue.front();
                jobQueue.pop_front();
            }

            job();
        }
    }

    std::thread thread;
    std::condition_variable queuePending;
    std::mutex queueMutex;
    std::list<job_t> jobQueue;
    bool wantExit = false;
    bool isExecuting = true;
};


template<size_t Threads, std::enable_if_t<(Threads > 0), int> = 0>
class JobPool
{
public:
    typedef std::function<void(size_t i)> job_t;

    /// <summary>Creates threads to queue jobs on.</summary>
    JobPool()
    {
        for (size_t i = Threads; i > 0; --i) {
            jobPool.push_back(std::make_unique<JobQueue>());
        }
        wait();
    }

    /// <summary>Creates save threads to queue jobs on.</summary>
    JobPool(catch_exceptions_t)
    {
        for (size_t i = Threads; i > 0; --i) {
            jobPool.push_back(std::make_unique<JobQueue>(catch_exceptions));
        }
        wait();
    }
    
    /// <summary>Creates threads to do the job on.</summary>
    /// <param name="job">function to be executed</param>
    /// <param name="begin">Start of pool</param>
    /// <param name="end">End of the pool</param>
    explicit JobPool(const job_t& job, const size_t begin, const size_t end)
    {
        const size_t jobs = end - begin;
        const size_t step = jobs / Threads + (jobs % Threads == 0 ? 0 : 1);
        for (size_t i = Threads; i > 0; --i) {
            threadPool.push_back(std::thread(&JobPool::execute, this, job, (i - 1) * step, std::min(i * step, jobs)));
        }
    }
    
    /// <summary>Creates threads to do the job on.</summary>
    /// <param name="job">function to be executed</param>
    /// <param name="begin">Start of pool</param>
    /// <param name="end">End of the pool</param>
    explicit JobPool(const job_t& job, const size_t begin, const size_t end, catch_exceptions_t)
    {
        const size_t jobs = end - begin;
        const size_t step = jobs / Threads + (jobs % Threads == 0 ? 0 : 1);
        for (size_t i = Threads; i > 0; --i) {
            threadPool.push_back(save_thread("JobPool", [=]() {
                execute(job, (i - 1) * step, std::min(i * step, jobs));
            }));
        }
    }

    /// <summary>Waits for the threads to finish and removes them.</summary>
    ~JobPool()
    {
        wait();
    }

    JobPool(const JobPool&) = delete;
    JobPool(JobPool&&) = default;
    JobPool& operator=(const JobPool&) = delete;
    JobPool& operator=(JobPool&&) = default;

    /// <summary>Adds job_t's to the job pool to be execute on separate threads.</summary>
    /// <param name="job">function to be executed</param>
    /// <param name="begin">Start of pool or end if `end` is null</param>
    /// <param name="end">End of the pool</param>
    void addJob(const job_t& job, const size_t begin, const size_t end = 0)
    {
        const size_t jobs = std::max(begin, end) - std::min(begin, end);
        const size_t step = jobs / Threads + (jobs % Threads == 0 ? 0 : 1);
        for (size_t i = 0; i < jobPool.size(); i++) {
            jobPool[i]->addJob([=]() {
                execute(job, i  * step, std::min((i + 1) * step, jobs));
            });
        }
    }

    /// <summary>Wait for all the jobs to finish.</summary>
    void wait()
    {
        for (std::thread& thread : threadPool) {
            thread.join();
        }
        threadPool.clear();
        for (const std::unique_ptr<JobQueue>& queue : jobPool) {
            while (queue->isBusy()) {}
        }
    }

private:
    void execute(const job_t& job, const size_t begin, const size_t end) const
    {
        for (size_t i = begin; i < end; i++) {
            job(i);
        }
    }

    std::vector<std::unique_ptr<JobQueue>> jobPool;
    std::vector<std::thread> threadPool;
};
