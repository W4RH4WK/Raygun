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

#include "raygun/audio/audio_source.hpp"

#include "raygun/assert.hpp"
#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"

namespace raygun::audio {

Source::Source()
{
    alGenSources(1, &m_source);
    if(RG().audioSystem().getError() != AL_NO_ERROR) {
        RAYGUN_FATAL("Unable to generate audio source");
    }

    setGain(1.0);

    alSourcef(m_source, AL_REFERENCE_DISTANCE, 100.0f);
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

Source::~Source()
{
    alDeleteSources(1, &m_source);
}

void Source::play()
{
    if(!m_sound) return;

    alSourcePlay(m_source);
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

void Source::play(std::shared_ptr<Sound> sound)
{
    if(m_sound == sound && isPlaying()) return;

    setSound(sound);
    play();
}

void Source::stop()
{
    alSourceStop(m_source);
}

bool Source::isPlaying() const
{
    ALenum state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

void Source::setGain(double gain)
{
    const auto effectVolume = RG().config().effectVolume;

    alSourcef(m_source, AL_GAIN, (float)(std::clamp(gain, 0.0, 1.0) * effectVolume));
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

void Source::setPitch(double factor)
{
    alSourcef(m_source, AL_PITCH, (float)factor);
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

void Source::setLoop(bool loop)
{
    alSourcei(m_source, AL_LOOPING, loop);
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

void Source::setPositional(bool positional)
{
    setSourceRelative(!positional);

    if(!positional) {
        move(zero());
    }

    m_positional = positional;
}

void Source::setSound(std::shared_ptr<Sound> sound)
{
    if(m_sound == sound) return;

    stop();

    m_sound = sound;

    alSourcei(m_source, AL_BUFFER, *m_sound);
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

void Source::move(const vec3& pos)
{
    if(!m_positional) return;

    alSource3f(m_source, AL_POSITION, pos.x, pos.y, pos.z);
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

void Source::setSourceRelative(bool relative)
{
    alSourcei(m_source, AL_SOURCE_RELATIVE, relative);
    RAYGUN_ASSERT(RG().audioSystem().getError() == AL_NO_ERROR);
}

} // namespace raygun::audio
