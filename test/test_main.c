#include "include/test_framework.h"

#include <stdio.h>

int main(void)
{
    if (!test_framework_init()) {
        fprintf(stderr, "test framework init failed\n");
        return 1;
    }

    int failed = test_run_all();

    test_framework_cleanup();

    return failed == 0 ? 0 : 1;
}
