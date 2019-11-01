/*
	OpenGL include file
	This file includes the appropriate opengl headers for the platform
*/
#pragma once

// Include platform specific OpenGL headers
#ifdef _WIN32
#include <GL/glew.h>
#include <GL/wglew.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#elif __linux
#include <GL/glew.h>
#include <GL/glxew.h>
#endif

