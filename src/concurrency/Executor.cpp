#include <afina/concurrency/Executor.h>
#include <algorithm>
//#include <iostream>

// to fix linking of this file added concurrency to Network/CMakelist.txt

namespace Afina {
namespace Concurrency {
void Executor::Start() {
    if (state == State::kStopped) {
        for (size_t i = 0; i < low_watermark; ++i) {

            threads.emplace_back(&(perform), this);
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
    state = State::kStopped;
}
void perform(Afina::Concurrency::Executor *ex) {
    std::function<void()> task;
    while (ex->state == Executor::State::kRun) {
        {
            std::unique_lock<std::mutex> lock(ex->mutex);
            ex->nfree++;
            auto time = std::chrono::system_clock::now() + std::chrono::milliseconds(ex->idle_time);
            if (!ex->empty_condition.wait_until(lock, time, [&]() { return ex->tasks.size() != 0; })) {
                ex->nfree--;
                if (ex->threads.size() > ex->low_watermark)
                    break;
            }

            task = (ex->tasks.front());
            ex->tasks.pop_front();
            ex->nfree--;
        }

        task();
    }

    {
        std::unique_lock<std::mutex> lock(ex->mutex);
        // ex->nthreads--;
        for (auto &i : ex->threads) {
            if (i.get_id() == std::this_thread::get_id()) {
                i.detach();
                i.swap(ex->threads.back());
                ex->threads.pop_back();
                //  ex->threads.erase(&i);
                if (ex->threads.size() == 0) {
                    ex->stop_cv.notify_all();
                }
                break;
            }
        }
    }
}

} // namespace Concurrency
} // namespace Afina
