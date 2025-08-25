#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <map>
#include <stack>
#include <glm/glm.hpp>
#include <type_traits>


#ifndef ENGINE_PLATFORM_WINDOWS
    #error Spike Engine supports only Windows!
#endif 

#define BIND_FUNCTION(x) std::bind(&x, this, std::placeholders::_1)

#define BIT(x) (1 << x)

#define ENUM_FLAGS_OPERATORS(type)                                                                                                  \
inline type operator~(type a) { return (type)~(std::underlying_type_t<type>)a;}                                                     \
inline type operator|(type a, type b) { return (type)((std::underlying_type_t<type>)a | (std::underlying_type_t<type>)b); }         \
inline type operator&(type a, type b) { return (type)((std::underlying_type_t<type>)a & (std::underlying_type_t<type>)b); }         \
inline type operator^(type a, type b) { return (type)((std::underlying_type_t<type>)a ^ (std::underlying_type_t<type>)b); }         \
inline type& operator|=(type& a, type b) { return (type&)((std::underlying_type_t<type>&)a |= (std::underlying_type_t<type>&)b); }  \
inline type& operator&=(type& a, type b) { return (type&)((std::underlying_type_t<type>&)a &= (std::underlying_type_t<type>&)b); }  \
inline type& operator^=(type& a, type b) { return (type&)((std::underlying_type_t<type>&)a ^= (std::underlying_type_t<type>&)b); } 

template<typename T>
inline bool EnumHasAllFlags(T flags, T contains) {

	return ((std::underlying_type_t<T>)flags & (std::underlying_type_t<T>)contains) == ((std::underlying_type_t<T>)contains);
}

template<typename T>
inline bool EnumHasAnyFlags(T flags, T contains) {

	return ((std::underlying_type_t<T>)flags & (std::underlying_type_t<T>)contains) != 0;
}

inline void HashCombine(size_t& hash1, size_t hash2)
{
	hash1 ^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
}

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using UniqueRef = std::unique_ptr<T>;

template<typename T, typename ... Args>
constexpr UniqueRef<T> CreateUniqueRef(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T>
constexpr WeakRef<T> CreateWeakRef(Ref<T> source)
{
	return source;
}

using Quaternion = glm::quat;
using Vec2 = glm::vec2;
using Vec2Int = glm::ivec2;
using Vec2Uint = glm::uvec2;
using Vec3 = glm::vec3;
using Vec3Int = glm::ivec3;
using Vec3Uint = glm::uvec3;
using Vec4 = glm::vec4;
using Vec4Int = glm::ivec4;
using Mat2x2 = glm::mat2;
using Mat3x3 = glm::mat3;
using Mat4x4 = glm::mat4;