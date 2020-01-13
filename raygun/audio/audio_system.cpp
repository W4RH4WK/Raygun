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

#include "raygun/audio/audio_system.hpp"

#include "raygun/raygun.hpp"

namespace raygun::audio {

AudioSystem::AudioSystem()
{
    m_device = alcOpenDevice(nullptr);
    if(!m_device) {
        RAYGUN_WARN("No audio device");
    }

    m_context = alcCreateContext(m_device, nullptr);
    if(!alcMakeContextCurrent(m_context)) {
        RAYGUN_FATAL("Unable to set up audio context");
    }

    setupMusic();

    for(int i = 0; i < m_soundEffects.size(); ++i) {
        m_soundEffects[i] = std::make_unique<Source>();
    }
}

AudioSystem::~AudioSystem()
{
    alcDestroyContext(m_context);
    alcCloseDevice(m_device);
}

void AudioSystem::update()
{
    const auto& scene = RG().scene();

    moveListener(scene.camera->transform());

    // Reposition audio sources
    scene.root->forEachEntity([](const Entity& entity) {
        if(entity.audioSource) {
            entity.audioSource->move(entity.transform().position);
        }
    });
}

void AudioSystem::moveListener(const Transform& transform)
{
    const auto pos = transform.position;
    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    RAYGUN_ASSERT(alGetError() == AL_NO_ERROR);

    const auto up = transform.up();
    const auto forward = transform.forward();
    ALfloat orientation[] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};
    alListenerfv(AL_ORIENTATION, orientation);
    RAYGUN_ASSERT(alGetError() == AL_NO_ERROR);
}

void AudioSystem::setupMusic()
{
    m_music = std::make_unique<Source>();
    m_music->setGain(RG().config().musicVolume);
    m_music->setLoop(true);
    m_music->setPositional(false);
}

void AudioSystem::playSoundEffect(std::shared_ptr<Sound> sound, double gain, std::optional<vec3> position)
{
    auto& source = *m_soundEffects[m_soundEffectsIndex++ % m_soundEffects.size()];

    source.stop();

    source.setPositional(position.has_value());
    source.move(position.value_or(zero()));

    source.setSound(std::move(sound));
    source.setGain(gain);
    source.play();
}

} // namespace raygun::audio
