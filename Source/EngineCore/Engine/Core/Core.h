#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>


#ifndef ENGINE_PLATFORM_WINDOWS
    #error Spike Engine supports only Windows!
#endif 

#define BIND_FUNCTION(x) std::bind(&x, this, std::placeholders::_1)

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}


template<typename T>
using URef = std::unique_ptr<T>;

template<typename T, typename ... Args>
constexpr URef<T> CreateURef(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}


template<typename T>
using WRef = std::weak_ptr<T>();