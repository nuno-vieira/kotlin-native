/*
 * Copyright 2010-2021 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

import kotlinx.cinterop.*
import kotlin.native.concurrent.freeze

fun setCErrorHandler(callback: CPointer<CFunction<(CPointer<ByteVar>) -> Unit>>?) {
    setUnhandledExceptionHook({
        throwable: Throwable ->
        memScoped {
            callback!!(throwable.toString().cstr.ptr)
        }
        kotlin.system.exitProcess(0)
    }.freeze())
}

fun throwException() {
    throw Error("Expected error")
}
