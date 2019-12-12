#include <afina/concurrency/Executor.h>
#include <algorithm>
#include <iostream>

// to fix linking of this file added concurrency to Network/CMakelist.txt

namespace Afina {
namespace Concurrency {
void Executor::Start() {
    std::unique_lock<std::mutex> lock(mutex);
    if (state == State::kStopped) {
        state = State::kRun;
        for (size_t i = 0; i < low_watermark; ++i) {
            std::thread t(&(perform), this);
            t.detach();

            nthreads++;
        }
    }
}

void Executor::Stop(bool await) {
    std::unique_lock<std::mutex> lock(mutex);
    if (state == State::kRun) {
        state = State::kStopping;

        if (await && nthreads > 0) {
            stop_cv.wait(lock, [&]() { return nthreads == 0; });
        } else if (await) {
            state = State::kStopped;
        }
    }
}
void perform(Afina::Concurrency::Executor *ex) {
    std::function<void()> task;

    bool haveTask(false);
    while (ex->state == Executor::State::kRun) {
        {
            std::unique_lock<std::mutex> lock(ex->mutex);
            ex->nfree++;
            auto time = std::chrono::system_clock::now() + std::chrono::milliseconds(ex->idle_time);
            if (!ex->empty_condition.wait_until(lock, time, [&]() { return ex->tasks.size() != 0; })) {
                if (ex->nthreads > ex->low_watermark) {
                    // no tasks and too many workers => exit
                    std::cout << "no tasks and too many workers => exit " << std::this_thread::get_id() << " "
                              << ex->nfree << std::endl;
                    ex->nfree--;
                    haveTask = false;
                    break;
                } else {
                    // no tasks, but low_watermark reached =>
                    // wait until timeout again
                    std::cout << "wait until timeout again " << std::this_thread::get_id() << " " << ex->nfree
                              << std::endl;

                    haveTask = false;
                }
            } else {
                // have task
                haveTask = true;
                std::cout << "have task " << std::this_thread::get_id() << " " << ex->nfree << std::endl;
                task = (ex->tasks.front());
                ex->tasks.pop_front();
            }
            ex->nfree--;
        }

        if (haveTask) {
            try {
                task();
            } catch (...) {
                std::cerr << "Critical error: task function pointer is null. Thread id: " << std::this_thread::get_id()  << std::endl;
                std::terminate();
            }
            haveTask = false;
        }
    }

    {
        std::unique_lock<std::mutex> lock(ex->mutex);

        {
            // thread finishing

            ex->nthreads--;

            if (ex->nthreads == 0 && ex->state == Executor::State::kStopping) {
                ex->state = Executor::State::kStopped;
                ex->stop_cv.notify_all();
            }
        }
    }
}

} // namespace Concurrency
} // namespace Afina
