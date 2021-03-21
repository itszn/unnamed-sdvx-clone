/* Used to generate precompiled header file for Main project*/
#include "stdafx.h"

// nuklear's nk_dtoa is inaccurate, even for exact values like 0.25f.
static inline void sprintf_dtoa(char(&buffer)[64 /* NK_MAX_NUMBER_BUFFER */], double d)
{
	Utility::BufferSprintf(buffer, "%g", d);
}

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION

#define NK_DTOA sprintf_dtoa

#ifdef EMBEDDED
#define NK_SDL_GLES2_IMPLEMENTATION
#include "../third_party/nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gles2.h"
#else
#define NK_SDL_GL3_IMPLEMENTATION
#include "../third_party/nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"
#endif