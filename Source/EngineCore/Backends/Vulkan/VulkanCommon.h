#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <Engine/Core/Log.h>


namespace Spike {

#define VK_CHECK(x)                                                                \
    do {                                                                           \
        VkResult err = x;                                                          \
        if(err)  {                                                                 \
            ENGINE_ERROR("Detected Vulkan Error: {}", string_VkResult(err));       \
            abort();                                                               \
        }                                                                          \
    } while (0)
}