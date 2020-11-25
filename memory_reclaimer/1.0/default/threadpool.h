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

#ifndef ANDROID_SYSTEM_MEMORY_RECLAIMER_V1_0_THREADPOOL_H
#define ANDROID_SYSTEM_MEMORY_RECLAIMER_V1_0_THREADPOOL_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace android::system::memory_reclaimer::implementation {

class ThreadPool {
   public:
    ThreadPool(int size);
    virtual ~ThreadPool();
    bool schedule(std::function<void()> const& task);
    void shutdown();

   private:
    void worker();

    std::vector<std::thread> threads_;
    std::deque<std::function<void()>> tasks_;
    bool shutdown_;

    std::mutex mutex_;
    std::condition_variable cond_;
};

}  // namespace android::system::memory_reclaimer::implementation

#endif  // ANDROID_SYSTEM_MEMORY_RECLAIMER_V1_0_THREADPOOL_H
