#include "SoundManager.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <random>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QStandardPaths>

#ifdef BREAKOUT_HAS_MULTIMEDIA
#include <QSoundEffect>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QIODevice>
#endif

namespace Breakout {

static QByteArray createWav(const std::vector<int16_t>& pcm, int sampleRate = 44100) {
    QByteArray wav;
    uint32_t dataSize = static_cast<uint32_t>(pcm.size() * sizeof(int16_t));
    uint32_t fileSize = 36 + dataSize;
    uint32_t byteRate = sampleRate * 2;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    uint16_t numChannels = 1;
    uint16_t audioFormat = 1;
    uint32_t subchunk1Size = 16;

    wav.append("RIFF", 4);
    wav.append(reinterpret_cast<const char*>(&fileSize), 4);
    wav.append("WAVE", 4);
    wav.append("fmt ", 4);
    wav.append(reinterpret_cast<const char*>(&subchunk1Size), 4);
    wav.append(reinterpret_cast<const char*>(&audioFormat), 2);
    wav.append(reinterpret_cast<const char*>(&numChannels), 2);
    wav.append(reinterpret_cast<const char*>(&sampleRate), 4);
    wav.append(reinterpret_cast<const char*>(&byteRate), 4);
    wav.append(reinterpret_cast<const char*>(&blockAlign), 2);
    wav.append(reinterpret_cast<const char*>(&bitsPerSample), 2);
    wav.append("data", 4);
    wav.append(reinterpret_cast<const char*>(&dataSize), 4);
    wav.append(reinterpret_cast<const char*>(pcm.data()), dataSize);
    return wav;
}

// Satisfying electric piano / acoustic chime "ting ting" synthesis
// Uses additive harmonics with fast acoustic attack and natural exponential decay
static std::vector<int16_t> synthPianoTing(float freq, float durationSec = 0.22f, float maxAmp = 22000.0f) {
    constexpr int sampleRate = 44100;
    int samples = static_cast<int>(sampleRate * durationSec);
    std::vector<int16_t> pcm(samples);

    float p1 = 0.0f; // Fundamental f
    float p2 = 0.0f; // Octave 2f
    float p3 = 0.0f; // Perfect 5th / 3rd harmonic 3f
    float p4 = 0.0f; // Bell shimmer partial 4.2f

    float inc1 = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
    float inc2 = inc1 * 2.0f;
    float inc3 = inc1 * 3.0f;
    float inc4 = inc1 * 4.2f;

    for (int i = 0; i < samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);

        p1 += inc1; if (p1 > 6.2831853f) p1 -= 6.2831853f;
        p2 += inc2; if (p2 > 6.2831853f) p2 -= 6.2831853f;
        p3 += inc3; if (p3 > 6.2831853f) p3 -= 6.2831853f;
        p4 += inc4; if (p4 > 6.2831853f) p4 -= 6.2831853f;

        // Rich piano/bell harmonic mixture
        float sample = std::sin(p1) * 0.70f +
                       std::sin(p2) * 0.25f +
                       std::sin(p3) * 0.12f +
                       std::sin(p4) * 0.06f;

        // Natural piano attack (3ms ramp) and warm exponential decay
        float attack = std::min(1.0f, t / 0.003f);
        float decay = std::exp(-t * 9.5f);
        float env = attack * decay;

        pcm[i] = static_cast<int16_t>(std::clamp(sample * env * maxAmp, -32767.0f, 32767.0f));
    }
    return pcm;
}

// Warm woody paddle bounce (acoustic marimba thud)
static std::vector<int16_t> synthPaddleThud(float freq = 220.0f, float durationSec = 0.09f) {
    constexpr int sampleRate = 44100;
    int samples = static_cast<int>(sampleRate * durationSec);
    std::vector<int16_t> pcm(samples);

    float p1 = 0.0f;
    float p2 = 0.0f;
    float inc1 = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
    float inc2 = inc1 * 2.0f;

    for (int i = 0; i < samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        p1 += inc1; if (p1 > 6.2831853f) p1 -= 6.2831853f;
        p2 += inc2; if (p2 > 6.2831853f) p2 -= 6.2831853f;

        float sample = std::sin(p1) * 0.85f + std::sin(p2) * 0.20f;
        float attack = std::min(1.0f, t / 0.002f);
        float decay = std::exp(-t * 22.0f);
        pcm[i] = static_cast<int16_t>(sample * attack * decay * 24000.0f);
    }
    return pcm;
}

// Soft subtle glass chime for wall bounces
static std::vector<int16_t> synthWallTick(float durationSec = 0.035f) {
    constexpr int sampleRate = 44100;
    int samples = static_cast<int>(sampleRate * durationSec);
    std::vector<int16_t> pcm(samples);

    float p = 0.0f;
    float inc = 2.0f * 3.14159265f * 1046.0f / static_cast<float>(sampleRate);

    for (int i = 0; i < samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        p += inc; if (p > 6.2831853f) p -= 6.2831853f;
        float decay = std::exp(-t * 55.0f);
        pcm[i] = static_cast<int16_t>(std::sin(p) * decay * 14000.0f);
    }
    return pcm;
}

static std::vector<int16_t> synthArpeggio(const std::vector<float>& freqs, float noteDuration, float maxAmp = 18000.0f) {
    constexpr int sampleRate = 44100;
    int noteSamples = static_cast<int>(sampleRate * noteDuration);
    int totalSamples = noteSamples * static_cast<int>(freqs.size());
    std::vector<int16_t> pcm(totalSamples);

    int idx = 0;
    for (float freq : freqs) {
        float phase = 0.0f;
        float inc = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
        for (int i = 0; i < noteSamples; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            phase += inc; if (phase > 6.2831853f) phase -= 6.2831853f;
            float env = std::min(1.0f, t / 0.003f) * std::exp(-t * 8.0f);
            pcm[idx++] = static_cast<int16_t>(std::sin(phase) * env * maxAmp);
        }
    }
    return pcm;
}

static std::vector<int16_t> synthNoise(float durationSec, float maxAmp = 18000.0f) {
    constexpr int sampleRate = 44100;
    int samples = static_cast<int>(sampleRate * durationSec);
    std::vector<int16_t> pcm(samples);
    std::mt19937 rng(54321);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    float lowFreqPhase = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float progress = static_cast<float>(i) / static_cast<float>(samples);
        float lowFreq = 160.0f * (1.0f - progress * 0.7f);
        lowFreqPhase += 2.0f * 3.14159265f * lowFreq / static_cast<float>(sampleRate);

        float noiseSample = dist(rng) * 0.6f + std::sin(lowFreqPhase) * 0.4f;
        float envelope = (1.0f - progress) * (1.0f - progress);
        pcm[i] = static_cast<int16_t>(noiseSample * envelope * maxAmp);
    }
    return pcm;
}

class SoundManager::Impl {
public:
    Impl() {
#ifdef BREAKOUT_HAS_MULTIMEDIA
        QString soundDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/breakout_sounds_v2/";
        QDir().mkpath(soundDir);

        auto saveAndLoad = [&](SoundEffect eff, const std::vector<int16_t>& pcm, const QString& filename) {
            QString path = soundDir + filename;
            QByteArray wav = createWav(pcm, 44100);
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(wav);
                file.close();
            }

            auto effect = std::make_unique<QSoundEffect>();
            effect->setSource(QUrl::fromLocalFile(path));
            effect->setVolume(0.8f);
            m_soundEffects[eff] = std::move(effect);
        };

        // Pentatonic C-major piano scale for satisfying musical brick breaks
        const float pianoNotes[8] = {
            523.25f, // C5
            587.33f, // D5
            659.25f, // E5
            783.99f, // G5
            880.00f, // A5
            1046.50f,// C6
            1174.66f,// D6
            1318.51f // E6
        };

        for (int i = 0; i < 8; ++i) {
            QString fname = QString("ting_%1.wav").arg(i);
            QString path = soundDir + fname;
            QByteArray wav = createWav(synthPianoTing(pianoNotes[i], 0.22f), 44100);
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(wav);
                file.close();
            }
            auto eff = std::make_unique<QSoundEffect>();
            eff->setSource(QUrl::fromLocalFile(path));
            eff->setVolume(0.8f);
            m_brickTings.push_back(std::move(eff));
        }

        saveAndLoad(SoundEffect::PaddleHit, synthPaddleThud(240.0f, 0.08f), "paddle_thud.wav");
        saveAndLoad(SoundEffect::BrickHit, synthPianoTing(783.99f, 0.20f), "brick_ting.wav");
        saveAndLoad(SoundEffect::WallBounce, synthWallTick(0.035f), "wall_tick.wav");
        saveAndLoad(SoundEffect::LaserFire, synthPianoTing(1046.50f, 0.07f, 16000.0f), "laser_chime.wav");
        saveAndLoad(SoundEffect::PowerUpCollect, synthArpeggio({523.25f, 659.25f, 783.99f, 1046.50f}, 0.06f), "powerup_piano.wav");
        saveAndLoad(SoundEffect::Explosion, synthNoise(0.24f), "explosion.wav");
        saveAndLoad(SoundEffect::BallLost, synthPianoTing(196.00f, 0.35f, 24000.0f), "lost_bass.wav");
        saveAndLoad(SoundEffect::LevelWon, synthArpeggio({523.25f, 659.25f, 783.99f, 1046.50f, 1318.51f}, 0.09f), "win_chime.wav");
        saveAndLoad(SoundEffect::GameOver, synthArpeggio({392.00f, 349.23f, 329.63f, 261.63f}, 0.12f), "gameover_piano.wav");
        saveAndLoad(SoundEffect::MenuSelect, synthPianoTing(880.00f, 0.06f, 16000.0f), "menu_ting.wav");
        saveAndLoad(SoundEffect::MenuChange, synthPianoTing(659.25f, 0.05f, 14000.0f), "menu_subtle.wav");
#endif
    }

    void play(SoundEffect eff, float volume, bool muted) {
        if (muted || volume <= 0.001f) return;

#ifdef BREAKOUT_HAS_MULTIMEDIA
        auto it = m_soundEffects.find(eff);
        if (it != m_soundEffects.end() && it->second) {
            it->second->setVolume(volume);
            it->second->play();
        }
#else
        (void)eff; (void)volume; (void)muted;
#endif
    }

    void playBrickTing(int comboStep, float volume, bool muted) {
        if (muted || volume <= 0.001f) return;

#ifdef BREAKOUT_HAS_MULTIMEDIA
        if (!m_brickTings.empty()) {
            size_t idx = static_cast<size_t>(std::abs(comboStep)) % m_brickTings.size();
            m_brickTings[idx]->setVolume(volume);
            m_brickTings[idx]->play();
        }
#else
        (void)comboStep; (void)volume; (void)muted;
#endif
    }

    void updateVolume(float vol) {
#ifdef BREAKOUT_HAS_MULTIMEDIA
        for (auto& pair : m_soundEffects) {
            if (pair.second) pair.second->setVolume(vol);
        }
        for (auto& eff : m_brickTings) {
            if (eff) eff->setVolume(vol);
        }
#else
        (void)vol;
#endif
    }

private:
#ifdef BREAKOUT_HAS_MULTIMEDIA
    std::map<SoundEffect, std::unique_ptr<QSoundEffect>> m_soundEffects;
    std::vector<std::unique_ptr<QSoundEffect>> m_brickTings;
#endif
};

SoundManager& SoundManager::instance() {
    static SoundManager inst;
    return inst;
}

SoundManager::SoundManager() : m_impl(std::make_unique<Impl>()) {}
SoundManager::~SoundManager() = default;

void SoundManager::setMuted(bool muted) {
    m_muted = muted;
    if (muted) {
        m_impl->updateVolume(0.0f);
    } else {
        m_impl->updateVolume(m_volume);
    }
}

void SoundManager::setVolume(float vol) {
    m_volume = std::clamp(vol, 0.0f, 1.0f);
    if (!m_muted) {
        m_impl->updateVolume(m_volume);
    }
}

void SoundManager::play(SoundEffect effect) {
    m_impl->play(effect, m_volume, m_muted);
}

void SoundManager::playBrickTing(int comboStep) {
    m_impl->playBrickTing(comboStep, m_volume, m_muted);
}

} // namespace Breakout
