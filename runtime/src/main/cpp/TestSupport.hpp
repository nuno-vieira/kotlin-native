/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

namespace kotlin {

#if KONAN_WINDOWS
// TODO: Figure out why creating many threads on windows is so slow.
constexpr int kDefaultThreadCount = 10;
#elif __has_feature(thread_sanitizer)
// TSAN has a huge overhead.
constexpr int kDefaultThreadCount = 10;
#else
constexpr int kDefaultThreadCount = 100;
#endif

// Scopely attaches the current thread to the mempory subsystem if the new MM is used.
// Does nothing for the legacy MM.
class MemoryInitGuard {
public:
    MemoryInitGuard() {
        if (CurrentMemoryModel == MemoryModel::kExperimental) {
            memoryState = InitMemory(false);
        }
    }
    ~MemoryInitGuard() {
        if (CurrentMemoryModel == MemoryModel::kExperimental) {
            DeinitMemory(memoryState, false);
        }
    }
private:
    MemoryState* memoryState = nullptr;
};

// Starts a new thread, attaches it to the memory subsystem and executes a given function in it.
template<typename F>
void RunInManagedThread(F&& function) {
    std::thread t([&function](){
        MemoryInitGuard guard;
        function();
    });
    t.join();
}

} // namespace kotlin
