#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <map>
//#include <iostream>
namespace Afina {
namespace Concurrency {

class Executor;
void perform(Afina::Concurrency::Executor *);
/**
 * # Thread pool
 */
class Executor
{

public:
  Executor(std::string name, size_t hight_watermark=10, size_t max_queue_size = 1000, size_t low_watermark = 1, size_t  idle_time=400) : low_watermark(low_watermark), hight_watermark(hight_watermark), max_queue_size(max_queue_size), idle_time(idle_time), nfree(0), nthreads(0)
  {
    //state=kRun;
    state = State::kStopped;
  } ;
  friend void perform(Executor *ex);

    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };


    ~Executor()
    {
      std::unique_lock<std::mutex> lock(mutex);
      if(state == State::kRun)
      {
        Stop(true);
      }

    }

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */

    void Start();


    void Stop(bool await = false);

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */


private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;
    std::condition_variable stop_cv;
    /**
     * Vector of actual threads that perorm execution
     */
    //std::vector<std::thread> threads;
    std::map<std::thread::id, std::thread> threads;
    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;

    size_t low_watermark;
    size_t hight_watermark;
    size_t max_queue_size;
    size_t idle_time;
    size_t nfree;
    size_t nthreads;
  //  size_t nthreads;

  public:
    template <typename F, typename... Types>
    bool Execute(F &&func, Types... args)
    {
      // Prepare "task"

      auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

      std::unique_lock<std::mutex> lock(this->mutex);
      if (state != State::kRun)
      {

          return false;
      }

      // Enqueue new task
      if(tasks.size() >= max_queue_size)
      {
        return false;
      }
      if(nfree==0 && threads.size() < hight_watermark)
      {

        //threads.emplace_back(&perform, this);
        std::thread t(&(perform), this);
        //threads[t.get_id()]=(std::move(t));
        auto id = t.get_id();
        threads.insert(std::move(std::make_pair(id, std::move(t))));

      }
      tasks.push_back(exec);

      empty_condition.notify_one();
      return true;
  }



};//executor


} // namespace Concurrency
} // namespace Afina

#endif // AFINA_CONCURRENCY_EXECUTOR_H
