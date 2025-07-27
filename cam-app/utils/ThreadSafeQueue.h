#ifndef CNSTREAM_THREADSAFE_QUEUE_HPP_
#define CNSTREAM_THREADSAFE_QUEUE_HPP_

#include <condition_variable>
#include <mutex>
#include <queue>

namespace cn {

    template <typename T>
    class ThreadSafeQueue {
    public:
        ThreadSafeQueue() = default;
        ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
        ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;

        bool TryPop(T& value);  // NOLINT

        void WaitAndPop(T& value);  // NOLINT

        bool WaitAndTryPop(T& value, const std::chrono::microseconds rel_time);  // NOLINT

        void Push(const T& new_value);  // NOLINT

        bool Empty() {
            std::lock_guard<std::mutex> lk(data_m_);
            return q_.empty();
        }

        uint32_t Size() {
            std::lock_guard<std::mutex> lk(data_m_);
            return q_.size();
        }

    private:
        std::mutex data_m_;
        std::queue<T> q_;
        std::condition_variable notempty_cond_;
    };

    template <typename T>
    bool ThreadSafeQueue<T>::TryPop(T& value) {  // NOLINT
        std::lock_guard<std::mutex> lk(data_m_);
        if (q_.empty()) {
            return false;
        }
        else {
            value = q_.front();
            q_.pop();
            return true;
        }
    }

    template <typename T>
    void ThreadSafeQueue<T>::WaitAndPop(T& value) {  // NOLINT
        std::unique_lock<std::mutex> lk(data_m_);
        notempty_cond_.wait(lk, [&] { return !q_.empty(); });
        value = q_.front();
        q_.pop();
    }

    template <typename T>
    bool ThreadSafeQueue<T>::WaitAndTryPop(T& value, const std::chrono::microseconds rel_time) {  // NOLINT
        std::unique_lock<std::mutex> lk(data_m_);
        if (notempty_cond_.wait_for(lk, rel_time, [&] { return !q_.empty(); })) {
            value = q_.front();
            q_.pop();
            return true;
        }
        else {
            return false;
        }
    }

    template <typename T>
    void ThreadSafeQueue<T>::Push(const T& new_value) {
        std::unique_lock<std::mutex> lk(data_m_);
        q_.push(new_value);
        lk.unlock();
        notempty_cond_.notify_one();
    }

}  // namespace cnstream

#endif  // CNSTREAM_THREADSAFE_QUEUE_HPP_

