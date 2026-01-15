#include "include/test_framework.h"

#include <lib/account.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_framework_initialized = false;

static TestEntry *g_tests = NULL;
static size_t g_test_count = 0;
static size_t g_test_cap = 0;

static void ensure_capacity(size_t need)
{
    if (g_test_cap >= need) {
        return;
    }

    size_t new_cap = (g_test_cap == 0) ? 8 : g_test_cap;
    while (new_cap < need) {
        new_cap *= 2;
    }

    TestEntry *new_buf = (TestEntry *)realloc(g_tests, new_cap * sizeof(TestEntry));
    if (!new_buf) {
        fprintf(stderr, "test_register: out of memory\n");
        exit(1);
    }

    g_tests = new_buf;
    g_test_cap = new_cap;
}

void test_register(TestFunc func, const char *title, const char *detail)
{
    if (!func || !title) {
        fprintf(stderr, "test_register: invalid arguments\n");
        exit(1);
    }

    ensure_capacity(g_test_count + 1);

    g_tests[g_test_count].func = func;
    g_tests[g_test_count].title = title;
    g_tests[g_test_count].detail = detail ? detail : "";
    g_test_count++;
}

static void print_banner(const char *title, const char *detail)
{
    printf("\n====================\n");
    printf("TEST: %s\n", title ? title : "(null)");
    if (detail && detail[0] != '\0') {
        printf("DETAIL: %s\n", detail);
    }
    printf("====================\n");
}

static bool test_account_save_load_roundtrip(void)
{
    ACCOUNT acc;
    memset(&acc, 0, sizeof(acc));

    generate_uuid_string(acc.UUID);
    acc.PASSWORD = 1234567;
    acc.BALANCE = 100;

    if (!save_account(&acc)) {
        return false;
    }

    ACCOUNT loaded;
    memset(&loaded, 0, sizeof(loaded));

    if (!load_account(acc.UUID, &loaded)) {
        return false;
    }

    if (strcmp(acc.UUID, loaded.UUID) != 0) {
        return false;
    }
    if (acc.PASSWORD != loaded.PASSWORD) {
        return false;
    }
    if (acc.BALANCE != loaded.BALANCE) {
        return false;
    }

    if (!delete_account_file(acc.UUID)) {
        return false;
    }

    return true;
}

static bool test_account_delete_file_then_load_fail(void)
{
    ACCOUNT acc;
    memset(&acc, 0, sizeof(acc));

    generate_uuid_string(acc.UUID);
    acc.PASSWORD = 7654321;
    acc.BALANCE = 999;

    if (!save_account(&acc)) {
        return false;
    }

    if (!delete_account_file(acc.UUID)) {
        return false;
    }

    ACCOUNT loaded;
    memset(&loaded, 0, sizeof(loaded));

    if (load_account(acc.UUID, &loaded)) {
        return false;
    }

    return true;
}

static bool test_hash_basic_ops(void)
{
    ACCOUNT acc;
    memset(&acc, 0, sizeof(acc));

    generate_uuid_string(acc.UUID);
    acc.PASSWORD = 1111111;
    acc.BALANCE = 1;

    if (!hash_insert_account(&acc)) {
        return false;
    }

    ACCOUNT *found = hash_find_account(acc.UUID);
    if (!found) {
        return false;
    }

    found->BALANCE = 2;
    if (!hash_update_account(found)) {
        return false;
    }

    ACCOUNT *found2 = hash_find_account(acc.UUID);
    if (!found2 || found2->BALANCE != 2) {
        return false;
    }

    if (!hash_delete_account(acc.UUID)) {
        return false;
    }

    if (hash_find_account(acc.UUID) != NULL) {
        return false;
    }

    return true;
}

bool test_framework_init(void)
{
    if (g_framework_initialized) {
        return true;
    }

    if (!init_account_system()) {
        fprintf(stderr, "account system init failed\n");
        return false;
    }

    g_tests = NULL;
    g_test_count = 0;
    g_test_cap = 0;

    test_register(test_account_save_load_roundtrip,
                  "account: save/load roundtrip",
                  "save_account then load_account and compare fields");

    test_register(test_account_delete_file_then_load_fail,
                  "account: delete then load should fail",
                  "delete_account_file then load_account must return false");

    test_register(test_hash_basic_ops,
                  "hash: insert/find/update/delete",
                  "basic CRUD on in-memory hash table");

    g_framework_initialized = true;
    return true;
}

void test_framework_cleanup(void)
{
    if (!g_framework_initialized) {
        return;
    }

    free(g_tests);
    g_tests = NULL;
    g_test_count = 0;
    g_test_cap = 0;

    cleanup_account_system();

    g_framework_initialized = false;
}

int test_run_all(void)
{
    int failed = 0;

    for (size_t i = 0; i < g_test_count; i++) {
        const TestEntry *t = &g_tests[i];
        print_banner(t->title, t->detail);

        bool ok = t->func();
        if (ok) {
            printf("RESULT: PASS\n");
        } else {
            printf("RESULT: FAIL\n");
            failed++;
        }
    }

    printf("\nSUMMARY: total=%zu failed=%d passed=%zu\n",
           g_test_count,
           failed,
           g_test_count - (size_t)failed);

    return failed;
}
