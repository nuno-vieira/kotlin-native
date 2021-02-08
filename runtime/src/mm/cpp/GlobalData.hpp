/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_GLOBAL_DATA_H
#define RUNTIME_MM_GLOBAL_DATA_H

#include "ObjectFactory.hpp"
#include "GlobalsRegistry.hpp"
#include "GC.hpp"
#include "StableRefRegistry.hpp"
#include "ThreadRegistry.hpp"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

// Global (de)initialization is undefined in C++. Use single global singleton to define it for simplicity.
class GlobalData : private Pinned {
public:
    static GlobalData& Instance() noexcept {
        // `GlobalData` is required during Kotlin's globals initialization. If we put `GlobalData`
        // as a regular global, then it's possible that Kotlin's globals are initialized before `GlobalData`.
        // So make it a local static instead.
        // TODO: This has a performance impact: every access has to check if `instance` is already initialized.
        static GlobalData instance;
        return instance;
    }

    ThreadRegistry& threadRegistry() noexcept { return threadRegistry_; }
    GlobalsRegistry& globalsRegistry() noexcept { return globalsRegistry_; }
    StableRefRegistry& stableRefRegistry() noexcept { return stableRefRegistry_; }
    ObjectFactory<GC>& objectFactory() noexcept { return objectFactory_; }
    GC& gc() noexcept { return gc_; }

private:
    GlobalData();
    ~GlobalData();

    ThreadRegistry threadRegistry_;
    GlobalsRegistry globalsRegistry_;
    StableRefRegistry stableRefRegistry_;
    ObjectFactory<GC> objectFactory_;
    GC gc_{objectFactory_};
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_GLOBAL_DATA_H
