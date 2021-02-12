/*
 * Copyright 2010-2021 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_MARK_AND_SWEEP_H
#define RUNTIME_MM_MARK_AND_SWEEP_H

#include <cstddef>

#include "Types.h"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

template <typename GC>
class ObjectFactory;

// Stop-the-world Mark-and-Sweep. Currently supports only single mutator.
class MarkAndSweep : private Pinned {
public:
    class ObjectData {
    public:
        enum class Color {
            kWhite = 0, // Initial color at the start of collection cycles. Objects with this color at the end of GC cycle are collected.
                        // All new objects are allocated with this color.
            kBlack, // Objects encountered during mark phase.
        };

        Color color() const noexcept { return color_; }
        void setColor(Color color) noexcept { color_ = color; }

    private:
        Color color_ = Color::kWhite;
    };

    class ThreadData : private Pinned {
    public:
        using ObjectData = MarkAndSweep::ObjectData;

        explicit ThreadData(MarkAndSweep& gc) noexcept : gc_(gc) {}
        ~ThreadData() = default;

        void SafePointFunctionEpilogue() noexcept;
        void SafePointLoopBody() noexcept;
        void SafePointExceptionUnwind() noexcept;
        void SafePointAllocation(size_t size) noexcept;

        void PerformFullGC() noexcept;

        void OnOOM(size_t size) noexcept;

    private:
        MarkAndSweep& gc_;
        size_t allocatedBytes_ = 0;
        size_t safePointsCounter_ = 0;
    };

    class Collection {
    public:
        Collection(ObjectFactory<MarkAndSweep>& objectFactory, KStdVector<ObjHeader*> rootset) noexcept :
            objectFactory_(objectFactory), stack_(std::move(rootset)) {}

        void Mark() noexcept;
        void Sweep() noexcept;

    private:
        ObjectFactory<MarkAndSweep>& objectFactory_;
        KStdVector<ObjHeader*> stack_;
    };

    explicit MarkAndSweep(ObjectFactory<MarkAndSweep>& objectFactory) noexcept : objectFactory_(objectFactory) {}
    ~MarkAndSweep() = default;

    void SetThreshold(size_t value) noexcept { threshold_ = value; }
    size_t GetThreshold() noexcept { return threshold_; }

    void SetAllocationThresholdBytes(size_t value) noexcept { allocationThresholdBytes_ = value; }
    size_t GetAllocationThresholdBytes() noexcept { return allocationThresholdBytes_; }

private:
    void PerformFullGC() noexcept;

    KStdUniquePtr<Collection> currentCollection_;
    ObjectFactory<MarkAndSweep>& objectFactory_;

    size_t threshold_ = 100;
    size_t allocationThresholdBytes_ = 1000;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_MARK_AND_SWEEP_H
