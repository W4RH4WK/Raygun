// The MIT License (MIT)
//
// Copyright (c) 2019,2020 The Raygun Authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#pragma once

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #define _CRT_SECURE_NO_WARNINGS
    #include <Windows.h>

    // Symbols that leak and cause problems:
    #undef FAR
    #undef NEAR
#endif

//////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <chrono>
#include <experimental/map>
#include <experimental/set>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <cassert>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>

//////////////////////////////////////////////////////////////////////////

#define VK_ENABLE_BETA_EXTENSIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

//////////////////////////////////////////////////////////////////////////

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

//////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/projection.hpp>

//////////////////////////////////////////////////////////////////////////

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_glfw.h>
#include <ImGui/imgui_impl_vulkan.h>
#include <ImGui/multiplot.h>

#include <imGuIZMO/imGuIZMO.h>

//////////////////////////////////////////////////////////////////////////

// PhysX complains if neither _DEBUG nor NDEBUG is set.
#if !defined(NDEBUG) && !defined(_DEBUG)
    #define _DEBUG
#endif

#include <PxPhysicsAPI.h>

//////////////////////////////////////////////////////////////////////////

#include <json/json.hpp>

//////////////////////////////////////////////////////////////////////////

#include <fmt/format.h>
#include <fmt/ostream.h> // operator<< support for logging

//////////////////////////////////////////////////////////////////////////

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

//////////////////////////////////////////////////////////////////////////

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

//////////////////////////////////////////////////////////////////////////

#include <AL/al.h>
#include <AL/alc.h>

#include <opus/opus_types.h>
#include <opus/opusfile.h>

//////////////////////////////////////////////////////////////////////////

namespace raygun {

namespace fs = std::filesystem;

using Clock = std::chrono::high_resolution_clock;

using string = std::string;
using string_view = std::string_view;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using quat = glm::quat;
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using uint = glm::uint;

using json = nlohmann::json;

constexpr vec3 UP = {0.0f, 1.0f, 0.0f};
constexpr vec3 RIGHT = {1.0f, 0.0f, 0.0f};
constexpr vec3 FORWARD = {0.0f, 0.0f, -1.0f};

template<typename T = vec3>
constexpr T zero()
{
    return glm::zero<T>();
}

} // namespace raygun
