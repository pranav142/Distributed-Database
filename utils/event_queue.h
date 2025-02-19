//
// Created by pknadimp on 1/20/25.
//

#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

// A thread safe implementation of a Queue
namespace utils {
    template<typename T>
    class EventQueue {
    public:
        EventQueue() = default;

        void push(T entry) { {
                std::scoped_lock lock(m_lock);
                m_queue.push(std::move(entry));
            }
            m_cv.notify_one();
        }

        // This is a blocking pop that will put the thread to sleep until data is
        // available
        T pop() {
            std::unique_lock lock(m_lock);
            // Wait until data is in the queue
            m_cv.wait(lock, [this] { return !m_queue.empty(); });
            T entry = std::move(m_queue.front());
            m_queue.pop();
            return entry;
        }

    private:
        std::queue<T> m_queue;
        std::mutex m_lock;
        std::condition_variable m_cv;
    };
} // namespace utils

#endif // EVENT_QUEUE_H
