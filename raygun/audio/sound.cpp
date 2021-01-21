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

#include "raygun/audio/sound.hpp"

#include "raygun/assert.hpp"
#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"

namespace raygun::audio {

Sound::Sound(string_view name, const fs::path& path) : m_name(name)
{
    auto error = 0;
    const auto file = op_open_file(path.string().c_str(), &error);
    if(error != 0) {
        RAYGUN_FATAL("Unable to open audio file ({}): {}", error, m_name);
    }

    const auto numChannels = op_channel_count(file, -1);
    if(numChannels > 2) {
        RAYGUN_FATAL("Invalid sound file with more than 2 channels ({}): {}", numChannels, m_name);
    }

    const auto numSamplesPerChannel = op_pcm_total(file, -1);

    std::vector<opus_int16> buf(numSamplesPerChannel * numChannels);
    for(size_t readSamples = 0; readSamples < buf.size();) {
        auto readSamplesPerChannel = op_read(file, buf.data() + readSamples, (int)(buf.size() - readSamples), nullptr);
        readSamples += readSamplesPerChannel * numChannels;
    }

    alGenBuffers(1, &m_buffer);
    if(RG().audioSystem().getError() != AL_NO_ERROR) {
        RAYGUN_FATAL("Unable to generate audio buffer");
    }

    const auto format = numChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

    alBufferData(m_buffer, format, buf.data(), (int)(buf.size() * sizeof(buf[0])), SAMPLE_RATE);
    if(RG().audioSystem().getError() != AL_NO_ERROR) {
        RAYGUN_FATAL("Unable to fill audio buffer");
    }
}

Sound::~Sound()
{
    alDeleteBuffers(1, &m_buffer);
}

} // namespace raygun::audio
