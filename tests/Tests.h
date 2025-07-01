#ifndef TESTS_H
#define TESTS_H


#include <vector>
#include "../shared_definitions.h"
#include "driver/config.h"
#include "driver/FixedMath/Fixed64.h"

#define BASIC_TEST_STEPS 1000
#define BASIC_TEST_STEPS_REDUCED 100
#define BASIC_TEST_RANGE_MAX 150

#define RESET   "\033[0m"
#define RED     "\033[31m" // Red
#define GREEN   "\033[32m" // Green

///
/// Class for creating and running highly customizable tests
///
class Tests {
public:
    static void Initialize();

    /// Test functions
    /// @param range_min minimum speed testing range
    /// @param range_max maximum speed testing range
    /// @return True - passed, False - Failed
    ///
    /// @note Use 'TestManager::Set{Parameter}()' to set parameters (once done, call TestManager::UpdateModesConstants())
    /// before calling 'TestManager::Accel{AccelMode}()',
    /// or call 'TestManager::Accel{AccelMode}({parameters})' and pass all the parameters
    static bool TestAccelLinear(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);
    static bool TestAccelPower(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);
    static bool TestAccelClassic(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);
    static bool TestAccelMotivity(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);
    static bool TestAccelNatural(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);
    static bool TestAccelJump(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);
    static bool TestAccelLUT(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);

    static bool TestAccelMode(AccelMode mode, float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);

    static std::array<bool, AccelMode_Count> TestAllBasic(float range_min = 0, float range_max = BASIC_TEST_RANGE_MAX);

private:
    //static CachedFunction functions[AccelMode_Count];

    static bool IsAccelValueGood(FP_LONG value);
    static bool IsCloseEnough(FP_LONG value1, float value2, float tolerance = 0.001f);
    static bool IsCloseEnoughRelative(FP_LONG value1, float value2, float tolerance = 0.00001f);

    static float lerp(float a, float b, float t);
};



#endif //TESTS_H
