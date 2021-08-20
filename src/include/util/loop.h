/*  Copyright (C) 2021, Hensoldt Cyber GmbH */
#ifndef LOOP_COUNT
#error "LOOP_COUNT not set"
#endif

#if __INCLUDE_LEVEL__ <= LOOP_COUNT
LOOP_ELEMENT
#include "loop.h"
#else
#undef LOOP_COUNT
#undef LOOP_ELEMENT
#endif