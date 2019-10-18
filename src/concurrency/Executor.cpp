#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {
  void perform(Executor *ex)
  {
    std::function<void()> task;
    while(ex->state == Executor::State::kRun)
    {
      {
        std::unique_lock<std::mutex> lock(ex->mutex);
        ex->nfree++;
        auto time = std::chrono::system_clock::now() + std::chrono::milliseconds(ex->idle_time);
        if(!ex->empty_condition.wait_until(lock, time, [&](){ return ex->tasks.size() == 0;})
           )
        {
          ex->nfree--;
          if(ex->threads.size() > ex->low_watermark)
            break;
        }

        task=(ex->tasks.front());
        ex->nfree--;
      }
      task();



    }

    {
      std::unique_lock<std::mutex> lock(ex->mutex);
      //ex->nthreads--;
      for(auto &i : ex->threads)
      {
        if(i.get_id()==std::this_thread::get_id())
        {
          i.detach();
          i.swap(ex->threads.back());
          ex->threads.pop_back();
          if(ex->threads.size()==0)
          {
            ex->stop_cv.notify_all();
          }
          break;
        }
      }


    }
  }


  template <typename F, typename... Types>
  bool Executor::Execute(F &&func, Types... args)
  {
      // Prepare "task"
      auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

      std::unique_lock<std::mutex> lock(this->mutex);
      if (state != State::kRun) {
          return false;
      }

      // Enqueue new task
      if(tasks.size() >= max_queue_size)
      {
        return false;
      }
      if(tasks.size()==0 || nfree==0)
      {
        threads.emplace_back(&(perform), this);

      }
      tasks.push_back(exec);
      empty_condition.notify_one();
      return true;
  }



}//ns concurrency
} // namespace Afina
