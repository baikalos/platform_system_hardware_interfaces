/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "threadpool.h"

namespace android::system::memory_reclaimer::implementation {

ThreadPool::ThreadPool(int size) {
    shutdown_ = false;
    threads_.resize(size);
    for (int i = 0; i < size; i++) {
        threads_[i] = std::thread(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        shutdown_ = true;
        cond_.notify_all();
    }

    for (int i = 0; i < threads_.size(); ++i) {
        threads_[i].join();
    }

    threads_.clear();
}

bool ThreadPool::schedule(std::function<void()> const& task) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (shutdown_) {
        // Don't accept new work if the threadpool is shutting down
        return false;
    }
    tasks_.emplace_back(task);
    cond_.notify_one();

    return true;
}

void ThreadPool::worker() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (true) {
        if (tasks_.empty()) {
            if (shutdown_) {
                break;
            } else {
                cond_.wait(lock);
            }
        } else {
            std::function<void()> task = tasks_.front();
            tasks_.pop_front();

            lock.unlock();
            task();
            lock.lock();
        }
    }
}

}  // namespace android::system::memory_reclaimer::implementation
