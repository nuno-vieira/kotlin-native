/*
 * Copyright 2010-2021 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "MarkAndSweep.hpp"

#include "../GlobalData.hpp"
#include "../ObjectFactory.hpp"
#include "../RootSet.hpp"
#include "../ThreadData.hpp"
#include "Natives.h"

using namespace kotlin;

namespace {

// This is copied verbatim from legacy MM.
// TODO: Come up with a better way to iterate object fields.
template <typename func>
inline void traverseObjectFields(ObjHeader* obj, func process) {
    const TypeInfo* typeInfo = obj->type_info();
    if (typeInfo != theArrayTypeInfo) {
        for (int index = 0; index < typeInfo->objOffsetsCount_; index++) {
            ObjHeader** location = reinterpret_cast<ObjHeader**>(reinterpret_cast<uintptr_t>(obj) + typeInfo->objOffsets_[index]);
            process(*location);
        }
    } else {
        ArrayHeader* array = obj->array();
        for (uint32_t index = 0; index < array->count_; index++) {
            process(*ArrayAddressOfElementAt(array, index));
        }
    }
}

} // namespace

void mm::MarkAndSweep::ThreadData::SafePointFunctionEpilogue() noexcept {
    if (gc_.GetThreshold() == 0 || (safePointsCounter_ + 1) % gc_.GetThreshold() == 0) {
        PerformFullGC();
    }
    ++safePointsCounter_;
}

void mm::MarkAndSweep::ThreadData::SafePointLoopBody() noexcept {
    if (gc_.GetThreshold() == 0 || (safePointsCounter_ + 1) % gc_.GetThreshold() == 0) {
        PerformFullGC();
    }
    ++safePointsCounter_;
}

void mm::MarkAndSweep::ThreadData::SafePointExceptionUnwind() noexcept {
    if (gc_.GetThreshold() == 0 || (safePointsCounter_ + 1) % gc_.GetThreshold() == 0) {
        PerformFullGC();
    }
    ++safePointsCounter_;
}

void mm::MarkAndSweep::ThreadData::SafePointAllocation(size_t size) noexcept {
    size_t allocationOverhead = gc_.GetAllocationThresholdBytes() == 0 ? allocatedBytes_ : allocatedBytes_ % gc_.GetAllocationThresholdBytes();
    if (allocationOverhead + size >= gc_.GetAllocationThresholdBytes()) {
        PerformFullGC();
    }
    allocatedBytes_ += size;
}

void mm::MarkAndSweep::ThreadData::PerformFullGC() noexcept {
    gc_.PerformFullGC();
}

void mm::MarkAndSweep::ThreadData::OnOOM(size_t size) noexcept {
    PerformFullGC();
}

void mm::MarkAndSweep::Collection::Mark() noexcept {
    while (!stack_.empty()) {
        ObjHeader* top = stack_.back();
        stack_.pop_back();

        if (top == nullptr) continue;
        // Handle initializing singleton.
        // TODO: Probably not the place for it.
        if (top == reinterpret_cast<ObjHeader*>(1)) continue;

        // TODO: Also weak reference needs to be accounted for.

        if (top->heap()) {
            auto& objectData = mm::ObjectFactory<MarkAndSweep>::NodeRef::From(top).GCObjectData();
            if (objectData.color() == ObjectData::Color::kBlack) {
                continue;
            }
            objectData.setColor(ObjectData::Color::kBlack);
        }

        traverseObjectFields(top, [this](ObjHeader* field) noexcept { stack_.push_back(field); });
    }
}

void mm::MarkAndSweep::Collection::Sweep() noexcept {
    auto iter = objectFactory_.Iter();
    for (auto it = iter.begin(); it != iter.end();) {
        auto& objectData = it->GCObjectData();
        switch (objectData.color()) {
            case ObjectData::Color::kBlack:
                objectData.setColor(ObjectData::Color::kWhite);
                ++it;
                continue;
            case ObjectData::Color::kWhite:
                // TODO: Finalizers, including destroying ExtraObjectData.
                iter.EraseAndAdvance(it);
                continue;
        }
    }
}

void mm::MarkAndSweep::PerformFullGC() noexcept {
    RuntimeAssert(currentCollection_ == nullptr, "Cannot have been called during another collection");

    KStdVector<ObjHeader*> rootset;
    for (auto& thread : mm::GlobalData::Instance().threadRegistry().Iter()) {
        thread.Publish();
        for (auto* object : mm::ThreadRootSet(thread)) {
            rootset.push_back(object);
        }
    }
    for (auto* object : mm::GlobalRootSet()) {
        rootset.push_back(object);
    }

    currentCollection_ = ::make_unique<Collection>(objectFactory_, std::move(rootset));
    currentCollection_->Mark();
    currentCollection_->Sweep();
    currentCollection_ = nullptr;
}
