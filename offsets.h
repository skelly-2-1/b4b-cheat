/*
@file

	offsets.h

@purpose

	Place where we can manage all our needed offsets
*/

#pragma once

#include <string>
#include <stdint.h>
#define OFFSET(t, o) extern t o
#include "offsets_def.h"
#undef OFFSET