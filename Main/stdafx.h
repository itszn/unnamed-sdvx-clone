/* Main and precompiled header file for Main project*/
#pragma once

// OpenGL headers
#include <Graphics/GL.hpp>

#ifdef _WIN32
// Windows Header Files:
#include <windows.h>
#include <tchar.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

// C RunTime Header Files
#include <stdlib.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <memory.h>
#include <time.h>

#include <cinttypes>
#include <ctime>

#include <functional>
#include <memory>
#include <string>
#include <queue>

#include <Shared/Shared.hpp>

// Graphics components
#include <Graphics/OpenGL.hpp>
#include <Graphics/Image.hpp>
#include <Graphics/ImageLoader.hpp>
#include <Graphics/Texture.hpp>
#include <Graphics/Material.hpp>
#include <Graphics/Mesh.hpp>
#include <Graphics/RenderQueue.hpp>
#include <Graphics/RenderState.hpp>
#include <Graphics/ParticleSystem.hpp>
#include <Graphics/MeshGenerators.hpp>
#include <Graphics/Font.hpp>
using namespace Graphics;

#include "archive.h"
#include "archive_entry.h"
#include "json.hpp"
#include "lua.hpp"

// NK imports
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#undef NK_IMPLEMENTATION
#ifdef EMBEDDED
#undef NK_SDL_GLES2_IMPLEMENTATION
#include "../third_party/nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gles2.h"
#else
#undef	NK_SDL_GL3_IMPLEMENTATION
#include "../third_party/nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"
#endif

constexpr int MAX_VERTEX_MEMORY = 512 * 1024;
constexpr int MAX_ELEMENT_MEMORY = 128 * 1024;
constexpr int FULL_FONT_TEXTURE_HEIGHT = 32768; //needed to load all CJK glyphs

#include "BasicDefinitions.hpp"

// Asset loading macro
#define CheckedLoad(__stmt) if(!(__stmt)){Logf("Failed to load asset [%s]", Logger::Severity::Error, #__stmt); return false; }
