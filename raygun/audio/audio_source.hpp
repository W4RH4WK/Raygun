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

#include "raygun/audio/sound.hpp"

namespace raygun::audio {

class Source {
  public:
    Source();
    ~Source();

    /// Restart if already playing.
    void play();

    /// Sets the given sound if different from current. Continues if already
    /// playing.
    void play(std::shared_ptr<Sound> sound);

    void stop();

    bool isPlaying() const;

    void setGain(double gain);
    void setPitch(double factor);
    void setLoop(bool loop);
    void setPositional(bool positional);

    void setSound(std::shared_ptr<Sound> sound);

    /// Move the audio source to the given position. Ignored if audio source is
    /// not positional.
    void move(const vec3& position);

  private:
    ALuint m_source;

    std::shared_ptr<Sound> m_sound;

    bool m_positional = true;

    void setSourceRelative(bool relative);
};

using UniqueSource = std::unique_ptr<Source>;

} // namespace raygun::audio
