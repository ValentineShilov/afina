#include <afina/concurrency/Executor.h>
#include <algorithm>
//#include <iostream>

// to fix linking of this file added concurrency to Network/CMakelist.txt

namespace Afina {
namespace Concurrency {
void Executor::Start() {
    if (state == State::kStopped) {
        for (size_t i = 0; i < low_watermark; ++i) {
          std::thread t(&(perform), this);
          //threads[t.get_id()]=(std::move(t));
          auto id = t.get_id();
          threads.insert(std::move(std::make_pair(id, std::move(t))));
        }
    }
    state = State::kRun;
}

void Executor::Stop(bool await) {
    std::unique_lock<std::mutex> lock(mutex);
    state = State::kStopping;

    if (await) {
        stop_cv.wait(lock, [&]() { return threads.size() == 0; });
    }

}
void perform(Afina::Concurrency::Executor *ex) {
    std::function<void()> task;
    //std::cout << "Thread created " << std::this_thread::get_id() <<std::endl;
    bool haveTask(false);
    while (ex->state == Executor::State::kRun)
    {
        {
            std::unique_lock<std::mutex> lock(ex->mutex);
            ex->nfree++;
            auto time = std::chrono::system_clock::now() + std::chrono::milliseconds(ex->idle_time);
            if (!ex->empty_condition.wait_until(lock, time, [&]() { return ex->tasks.size() != 0; })) {
                if (ex->threads.size() > ex->low_watermark)
                {
                   //no tasks and too many workers => exit
                    //std::cout << "no tasks and too many workers => exit " << std::this_thread::get_id() <<" " << ex->nfree <<std::endl;
                    ex->nfree--;
                    haveTask=false;
                    break;
                }
                else
                {
                      //no tasks, but low_watermark reached =>
                      //wait until timeout again
                      //std::cout << "wait until timeout again " << std::this_thread::get_id() <<" " <<ex->nfree <<std::endl;

                      haveTask=false;
                }
            }
            else
            {
              //have task
              haveTask=true;
              //std::cout << "have task " << std::this_thread::get_id() <<" " <<ex->nfree <<std::endl;
              task = (ex->tasks.front());
              ex->tasks.pop_front();

            }
            ex->nfree--;
        }

        if(haveTask)
        {
          task();
          haveTask=false;
        }
    }

    {
        std::unique_lock<std::mutex> lock(ex->mutex);
        // ex->nthreads--;

        auto it(ex->threads.find(std::this_thread::get_id())) ;
        if(it != ex->threads.end())
        {
          // found
          auto &i(it->second);
          i.detach();
          ex->threads.erase(it);
          //  ex->threads.erase(&i);
          if (ex->threads.size() == 0)
          {
              ex->state = Executor::State::kStopped;
              ex->stop_cv.notify_all();
<<<<<<< HEAD

=======
              ex->state = Executor::State::kStopped;
>>>>>>> 6421d1e44dc9877ef9808062d326318d5ad50ce1
          }

        }

    }
}

} // namespace Concurrency
} // namespace Afina
