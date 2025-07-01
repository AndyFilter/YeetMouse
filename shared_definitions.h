#pragma once

#ifndef SHARED_DEFINITIONS_H
#define SHARED_DEFINITIONS_H

enum AccelMode {
    AccelMode_Current = 0, // Mainly used in GUI, denotes lack of a curve on the driver side
    AccelMode_Linear = 1,
    AccelMode_Power = 2,
    AccelMode_Classic = 3,
    AccelMode_Motivity = 4,
    AccelMode_Natural = 5,
    AccelMode_Jump = 6,
    AccelMode_Lut = 7,
    AccelMode_CustomCurve = 8,
    AccelMode_Count,
};

#endif
