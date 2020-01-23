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

#include "raygun/assert.hpp"
#include "raygun/audio/audio_source.hpp"
#include "raygun/audio/sound.hpp"
#include "raygun/logging.hpp"
#include "raygun/transform.hpp"

namespace raygun::audio {

class AudioSystem {
  public:
    AudioSystem();
    ~AudioSystem();

    Source& music() { return *m_music; }

    void setupDefaultSources();

    void update();

    void playSoundEffect(std::shared_ptr<Sound> sound, double gain = 1.0, std::optional<vec3> position = {});

    /// Wrapper around alGetError, returns AL_NO_ERROR if sound device is
    /// missing.
    ALenum getError() const;

  private:
    ALCdevice* m_device = nullptr;
    ALCcontext* m_context = nullptr;

    UniqueSource m_music;

    unsigned m_soundEffectsIndex = 0;
    std::array<UniqueSource, 32> m_soundEffects = {};

    void moveListener(const Transform& transform);

    void setupMusic();
};

using UniqueAudioSystem = std::unique_ptr<AudioSystem>;

} // namespace raygun::audio
