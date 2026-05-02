#pragma once

#define IM_VEC2_CLASS_EXTRA \
inline bool operator<=(const ImVec2& other) const { return x <= other.x && y <= other.y; } \
inline bool operator>=(const ImVec2& other) const { return x >= other.x && y >= other.y; }

// imgui
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <imgui_stdlib.h>

// glfw3
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3.h>
#include <glfw3native.h>

// stb
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

// stdlib
#include <memory>
#include <vector>
#include <functional>
#include <array>
#include <algorithm>
#include <random>


#ifdef _DEBUG
#include <chrono>

#define INT3 __debugbreak()
#define ASSERT(cond, ...) IM_ASSERT(cond)
#else
#define ASSERT(cond, returnvalue) if (!(cond)) { return returnvalue; }
#endif