#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*TestFunc)(void);

typedef struct TestEntry {
    TestFunc func;
    const char *title;
    const char *detail;
} TestEntry;

bool test_framework_init(void);
void test_framework_cleanup(void);

void test_register(TestFunc func, const char *title, const char *detail);

int test_run_all(void);

#ifdef __cplusplus
}
#endif

#endif
