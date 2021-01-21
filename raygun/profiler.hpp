// The MIT License (MIT)
//
// Copyright (c) 2019-2021 The Raygun Authors.
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

#include "raygun/vulkan_context.hpp"

namespace raygun {

enum class TimestampQueryID : uint32_t {

#define GPU_TIME(_name, _inchart, _color) _name##Start, _name##End,
#include "raygun/profiler.def"
    Count,
};

class Profiler {
  public:
    Profiler();

    void writeTimestamp(vk::CommandBuffer& cmdBuffer, TimestampQueryID id, vk::PipelineStageFlagBits pipelineStage = vk::PipelineStageFlagBits::eAllCommands);
    double getTimeRangeMS(TimestampQueryID begin, TimestampQueryID end) const;

    // Needs to be called before rendering a frame, with the cmd buffer
    // containing the first executed commands.
    void resetVulkanQueries(vk::CommandBuffer& cmdBuffer);

    void startFrame();
    void endFrame();

    void doUI() const;

  private:
    static constexpr uint32_t QUERY_BUFFER_FRAMES = 8;
    static constexpr uint32_t MAX_TIMESTAMP_QUERIES = (uint32_t)TimestampQueryID::Count;
    static constexpr uint32_t STATISTIC_FRAMES = 500;

    Clock::time_point frameStartTime = Clock::time_point::min();

    uint32_t curQueryFrame = 0;
    uint32_t prevQueryFrame() const;
    void incFrame();

    uint64_t getTimestamp(TimestampQueryID id) const;

    vk::UniqueQueryPool timestampQueryPool;
    std::array<uint64_t, MAX_TIMESTAMP_QUERIES* QUERY_BUFFER_FRAMES> timestampQueryResults = {};
    uint32_t timestampValidBits;

    std::array<float, STATISTIC_FRAMES> cpuTimes = {};
    std::array<float, STATISTIC_FRAMES> totalTimes = {};

#define GPU_TIME(_name, _inchart, _color) std::array<float, STATISTIC_FRAMES> _name##Times = {};
#include "raygun/profiler.def"

    uint32_t curStatFrame = 0;
    uint32_t prevStatFrame() const;

    VulkanContext& vc;
};

using UniqueProfiler = std::unique_ptr<Profiler>;

} // namespace raygun
