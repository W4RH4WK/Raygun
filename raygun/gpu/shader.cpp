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

#include "raygun/gpu/shader.hpp"

#include "raygun/raygun.hpp"
#include "raygun/utils/io_utils.hpp"

namespace raygun::gpu {

Shader::Shader(string_view, const fs::path& path)
{
    auto& vc = RG().vc();

    const auto code = io::readFile(path);

    vk::ShaderModuleCreateInfo info = {};
    info.setCodeSize(code.size());
    info.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

    shaderModule = vc.device->createShaderModuleUnique(info);
    vc.setObjectName(*shaderModule, path.stem().string());
}

vk::PipelineShaderStageCreateInfo Shader::shaderStageInfo(vk::ShaderStageFlagBits shaderStages) const
{
    vk::PipelineShaderStageCreateInfo info = {};

    info.setModule(*shaderModule);
    info.setStage(shaderStages);

    // GLSL only supports one entry point.
    info.setPName("main");

    return info;
}

void recompileAllShaders()
{
    const auto shaderDir = fs::path{"resources/shaders"};
    const std::set<fs::path> extensions = {".rgen", ".rint", ".rahit", ".rchit", ".rmiss", ".rcall"};

    for(const auto& entry: fs::directory_iterator(shaderDir)) {
        if(extensions.find(entry.path().extension()) == extensions.end()) continue;

        const auto cmd = fmt::format("glslc.exe -o {0}.spv {0}", entry.path());
        if(system(cmd.c_str()) != 0) {
            RAYGUN_WARN("Compiling {} failed", entry.path());
        }
        else {
            RAYGUN_INFO("Compiled {}", entry.path());
        }
    }
}

} // namespace raygun::gpu
