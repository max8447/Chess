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
#include <chrono>
#include <map>
#include <unordered_set>
#include <execution>
#include <cstdint>
#include <type_traits>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(*(arr)))
#define INT3 __debugbreak()

// #define MAX_SPEED

#if defined(_DEBUG) && defined(MAX_SPEED)
#error MAX_SPEED can't be enabled when in debug configuration!
#endif

#ifdef _DEBUG
#define ASSERT(cond, ...) IM_ASSERT(cond)
#else // _DEBUG
#ifdef MAX_SPEED
#define ASSERT(...)
#else // MAX_SPEED
#define ASSERT(cond, returnvalue) if (!(cond)) { INT3; return returnvalue; }
#endif // MAX_SPEED
#endif // _DEBUG

#define ENUM_OPERATORS(EEnumClass)																																		\
																																										\
inline constexpr EEnumClass& operator&=(EEnumClass& Left, EEnumClass Right)																								\
{																																										\
	return (EEnumClass&)((std::underlying_type<EEnumClass>::type&)(Left) &= (std::underlying_type<EEnumClass>::type)(Right));											\
}																																										\
inline constexpr EEnumClass& operator|=(EEnumClass& Left, EEnumClass Right)																								\
{																																										\
	return (EEnumClass&)((std::underlying_type<EEnumClass>::type&)(Left) |= (std::underlying_type<EEnumClass>::type)(Right));											\
}																																										\
inline constexpr EEnumClass& operator&=(EEnumClass& Left, int Right)																									\
{																																										\
	return (EEnumClass&)((std::underlying_type<EEnumClass>::type&)(Left) &= Right);																						\
}																																										\
inline constexpr EEnumClass& operator|=(EEnumClass& Left, int Right)																									\
{																																										\
	return (EEnumClass&)((std::underlying_type<EEnumClass>::type&)(Left) |= Right);																						\
}

// https://stackoverflow.com/questions/72336579/good-way-of-popping-the-least-signifigant-bit-and-returning-the-index
static int pop_lsb(uint64_t& b)
{
    int idx = std::countr_zero(b);
    b &= b - 1;
    return idx;
}
