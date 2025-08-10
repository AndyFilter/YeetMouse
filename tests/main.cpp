#include <iostream>

#include "TestManager.h"
#include "Tests.h"

int main() {
    Tests::Initialize();
    TestManager::Initialize();

    auto basic_test_res = Tests::TestAllBasic();

    int bad_sum = 0;
    for (int i = 0; i < basic_test_res.size(); i++) {
        if (!basic_test_res[i]) {
            fprintf(stderr, "Test failed for '%s' mode\n", AccelMode2String(static_cast<AccelMode>(i)).c_str());
            bad_sum++;
        }
    }

    if (bad_sum == 0) {
        printf(GREEN"All tests passed!\n" RESET);
    }
    else {
        printf(RED"%i %s failed!\n", bad_sum, (bad_sum == 1) ? "test" : "tests");
    }

    // Tests::TestFixedPointArithmetic();

    return 0;
}