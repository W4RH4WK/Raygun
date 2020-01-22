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

#include "raygun/logging.hpp"

namespace raygun::glfw {

class Runtime {
  public:
    Runtime()
    {
        glfwSetErrorCallback([](int error, const char* desc) { RAYGUN_ERROR("GLFW: {} ({})", desc, error); });

        if(glfwInit()) {
            RAYGUN_INFO("GLFW initialized");
        }
        else {
            RAYGUN_FATAL("Unable to initialize GLFW");
        }

        if(!glfwVulkanSupported()) {
            RAYGUN_FATAL("GLFW not supporting Vulkan");
        }
    }

    ~Runtime() { glfwTerminate(); }

    std::vector<const char*> vulkanExtensions() const
    {
        uint32_t count;
        const char** exts = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char*> result(count);
        std::copy(exts, exts + count, result.begin());
        return result;
    }

    void pollEvents()
    {
        RAYGUN_TRACE("Polling window system events");

        glfwPollEvents();
    }
};

using UniqueRuntime = std::unique_ptr<Runtime>;

struct Icon {
    Icon(const fs::path& pgmFile)
    {
        std::ifstream in{pgmFile, std::ios::binary};
        if(!in) {
            RAYGUN_FATAL("Could not load image: {}", pgmFile);
        }

        std::string magicNumber{2, '\0'};
        in.read(magicNumber.data(), magicNumber.size());
        if(magicNumber != "P5") {
            RAYGUN_FATAL("Invalid image format (not PGM binary): {}", pgmFile);
        }

        in >> image.width >> image.height;

        int maxValue;
        in >> maxValue;
        if(maxValue > 255) {
            RAYGUN_FATAL("Only 8-bit PGM is supported: {}", pgmFile);
        }

        in.ignore(1); // skip newline

        for(auto i = 0; i < image.width * image.height; i++) {
            unsigned char value;
            in.read((char*)&value, 1);

#ifdef NDEBUG
            constexpr unsigned char base[] = {0x6c, 0x7a, 0x90};
#else
            constexpr unsigned char base[] = {0xd7, 0x39, 0x38};
#endif

            // The image data is used as alpha channel with a fixed base color.
            const auto alpha = (unsigned char)(maxValue - value);
            data.insert(data.end(), {base[0], base[1], base[2], alpha});
        }

        image.pixels = data.data();
    }

    GLFWimage image = {};
    std::vector<unsigned char> data;
};

} // namespace raygun::glfw
