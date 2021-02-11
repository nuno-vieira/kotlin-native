#include "testlib_api.h"

#include <stdio.h>

void errorHandler(const char* str) {
    printf("Error handler: %s\n", str);
}

int main() {
    testlib_symbols()->kotlin.root.setCErrorHandler(&errorHandler);
    testlib_symbols()->kotlin.root.throwException();

    return 0;
}
