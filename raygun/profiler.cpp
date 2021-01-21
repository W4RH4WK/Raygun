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

#include "raygun/profiler.hpp"

#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"
#include "raygun/render/render_system.hpp"
#include "raygun/utils/array_utils.hpp"
#include "raygun/vulkan_context.hpp"

namespace raygun {

Profiler::Profiler() : vc(RG().vc())
{
    vk::QueryPoolCreateInfo timestampQPCI;
    timestampQPCI.setQueryType(vk::QueryType::eTimestamp);
    timestampQPCI.setQueryCount(MAX_TIMESTAMP_QUERIES * QUERY_BUFFER_FRAMES);
    timestampQueryPool = vc.device->createQueryPoolUnique(timestampQPCI);

    timestampValidBits = vc.physicalDevice.getQueueFamilyProperties()[0].timestampValidBits;

    RAYGUN_INFO("Profiler initialized");
}

uint64_t Profiler::getTimestamp(TimestampQueryID id) const
{
    return timestampQueryResults[(uint32_t)id];
}

double Profiler::getTimeRangeMS(TimestampQueryID begin, TimestampQueryID end) const
{
    return (getTimestamp(end) - getTimestamp(begin)) / (1000.0 * 1000.0);
}

void Profiler::writeTimestamp(vk::CommandBuffer& cmdBuffer, TimestampQueryID id,
                              vk::PipelineStageFlagBits pipelineStage /*= vk::PipelineStageFlagBits::eAllCommands*/)
{
    cmdBuffer.writeTimestamp(pipelineStage, *timestampQueryPool, (uint32_t)id + MAX_TIMESTAMP_QUERIES * curQueryFrame);
}

void Profiler::resetVulkanQueries(vk::CommandBuffer& cmdBuffer)
{
    cmdBuffer.resetQueryPool(*timestampQueryPool, curQueryFrame * MAX_TIMESTAMP_QUERIES, MAX_TIMESTAMP_QUERIES);
}

void Profiler::startFrame()
{
    if(frameStartTime == Clock::time_point::min()) {
        frameStartTime = Clock::now();
        return;
    }

    // Get GPU times from device
    {
        const auto result =
            vc.device->getQueryPoolResults(*timestampQueryPool, prevQueryFrame() * MAX_TIMESTAMP_QUERIES, MAX_TIMESTAMP_QUERIES, timestampQueryResults.size(),
                                           timestampQueryResults.data(), sizeof(uint64_t), vk::QueryResultFlagBits::e64);

        if(result != vk::Result::eSuccess && result != vk::Result::eNotReady) {
            RAYGUN_INFO("Unable to get query pool results");
            return;
        }
    }

    // Transform GPU times from from ticks (plus some potential invalid bits) to valid time units
    std::transform(timestampQueryResults.cbegin(), timestampQueryResults.cend(), timestampQueryResults.begin(), [&](uint64_t ts) {
        return glm::bitfieldExtract<std::uint64_t>(ts, 0, timestampValidBits) / (uint64_t)vc.physicalDeviceProperties.limits.timestampPeriod;
    });

    // Store current GPU times in statistics array
#define GPU_TIME(_name, _inchart, _color) _name##Times[curStatFrame] = (float)getTimeRangeMS(TimestampQueryID::_name##Start, TimestampQueryID::_name##End);
#include "raygun/profiler.def"

    float frameTimeMs = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - frameStartTime).count() / 1000.f;
    totalTimes[curStatFrame] = frameTimeMs;

    incFrame();
    frameStartTime = Clock::now();
}

void Profiler::endFrame()
{
    float frameTimeMs = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - frameStartTime).count() / 1000.f;
    cpuTimes[curStatFrame] = frameTimeMs;
}

void Profiler::doUI() const
{
    ImGui::Begin("Profiling");

    string gpuTTexts = "GPU times";
    string gpuTMeans = "     mean";

#define GPU_TIME(_name, _inchart, _color) \
    gpuTTexts += fmt::format(" | {}: {:5.2f}", #_name, _name##Times[prevStatFrame()]); \
    gpuTMeans += fmt::format(" | {}: {:5.2f}", #_name, utils::mean(_name##Times));
#include "raygun/profiler.def"

    ImGui::Text("CPU times | %s: %5.2f | %s: %5.2f", "CPU", cpuTimes[prevStatFrame()], "Total", totalTimes[prevStatFrame()]);
    ImGui::Text("     mean | %s: %5.2f | %s: %5.2f", "CPU", utils::mean(cpuTimes), "Total", utils::mean(totalTimes));

    ImGui::Text("%s", gpuTTexts.c_str());
    ImGui::Text("%s", gpuTMeans.c_str());

    float smoothedMax = 0.f;
    for(size_t i = 1; i < STATISTIC_FRAMES - 1; ++i) {
        smoothedMax = std::max(smoothedMax, std::min(totalTimes[i - 1], totalTimes[i]));
    }

    std::vector<const char*> names{"Total time", "CPU time"};
    std::vector<ImColor> colors{ImColor(0.9f, 0.9f, 0.9f), ImColor(0.6f, 0.6f, 0.6f)};
    std::vector<const void*> datas{totalTimes.data(), cpuTimes.data()};

#define GPU_TIME(_name, _inchart, _color) \
    if constexpr(_inchart) { \
        names.push_back(#_name); \
        colors.push_back(_color); \
        datas.push_back(_name##Times.data()); \
    }
#include "raygun/profiler.def"

    ImGui::PlotMultiLines(
        "Frametimes", (int)names.size(), names.data(), colors.data(),
        [&](const void* data, int idx) { return static_cast<const float*>(data)[(prevStatFrame() + idx) % STATISTIC_FRAMES]; }, datas.data(), STATISTIC_FRAMES,
        0, smoothedMax + 1, ImVec2(STATISTIC_FRAMES, 200));
    ImGui::End();
}

uint32_t Profiler::prevQueryFrame() const
{
    return (int)curQueryFrame - 1 < 0 ? QUERY_BUFFER_FRAMES - 1 : curQueryFrame - 1;
}

uint32_t Profiler::prevStatFrame() const
{
    return (int)curStatFrame - 1 < 0 ? STATISTIC_FRAMES - 1 : curStatFrame - 1;
}

void Profiler::incFrame()
{
    curQueryFrame++;
    if(curQueryFrame >= QUERY_BUFFER_FRAMES) {
        curQueryFrame = 0;
    }

    curStatFrame++;
    if(curStatFrame >= STATISTIC_FRAMES) {
        curStatFrame = 0;
    }
}

} // namespace raygun
