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
#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

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