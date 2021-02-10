/*
 * Copyright 2010-2021 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_CURRENT_EXCEPTION_H
#define RUNTIME_MM_CURRENT_EXCEPTION_H

#include "KAssert.h"
#include "Utils.hpp"

struct ObjHeader;

namespace kotlin {
namespace mm {

class CurrentException : private Pinned {
public:
    using Iterator = ObjHeader**;

    void SetCurrentException(ObjHeader* exception) noexcept {
        RuntimeAssert((currentException_ == nullptr && exception != nullptr) || (currentException_ != nullptr && exception == nullptr),
            "Unexpected exception state currentException=%p newException=%p", currentException_, exception);
        currentException_ = exception;
    }

    ObjHeader* currentException() noexcept { return currentException_; }

    Iterator begin() noexcept { return &currentException_; }
    Iterator end() noexcept { return begin() + 1; }

private:
    ObjHeader* currentException_ = nullptr;
};

}
}


#endif // RUNTIME_MM_CURRENT_EXCEPTION_H
