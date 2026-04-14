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

    bool arithmetic_test = Tests::TestFixedPointArithmetic();

    if (bad_sum == 0 && arithmetic_test) {
        printf(GREEN"All tests passed!\n\n" RESET);
    } else {
        printf(RED"%i %s failed!\n\n", bad_sum, (bad_sum == 1) ? "test" : "tests");
    }

    const int exit_code = (bad_sum == 0 && arithmetic_test) ? 0 : 1;

    return exit_code;
}
