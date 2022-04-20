//===--------------------------------------------------------------------------------------------===
// math.h - Maths utils
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2022 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#ifndef _SHADES_MATH_H_
#define _SHADES_MATH_H_

#include <math.h>

typedef struct {
    float x, y;
} vect2_t;

#define VECT2(x, y)         ((vect2_t){x, y})
#define NULL_VECT2          VECT2(NAN, NAN)
#define IS_NULL_VECT2(v)    (isnan((a).x))

#define UNUSED(x)           ((void)x)

#endif /* ifndef _SHADES_MATH_H_ */
