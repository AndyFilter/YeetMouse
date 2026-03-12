import os
from time import sleep

sleep(0.2)

header_file = open("driver/accel_modes.h", "r")
header_contents = header_file.read()
header_file.close()
header_contents = """#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
""" + header_contents + """

#ifdef __cplusplus
}
#endif"""

sleep(0.1) # https://preview.redd.it/just-add-sleep-v0-iak44ctwyfba1.jpg?auto=webp&s=352bb4ea528518a97a36600b1271535cf9852b4a

open("driver/accel_modes.h", "w").write(header_contents)

# fixed_point_file = open("driver/FixedMath/Fixed64.h", "r")
# fixed_point_contents = fixed_point_file.read()
# fixed_point_file.close()
#
# fixed_point_contents.replace("// static float FP64_ToFloat(FP_LONG v) {", "")
#
# open("driver/FixedMath/Fixed64.h", "w").write(fixed_point_contents)