#include "AudioManager.h"

#include <algorithm>
#include <iostream>

// AudioManager la noi DUY NHAT ap dung mix cho asset. setUserVolumes() nhan
// gia tri % tu Player Options; setBaseVolume() nhan he so 0.0..1.0 tu mixer.
// applyVolumes() nhan hai gia tri nay de dat:
//   SFX final = userSfxVolume_ * baseVolumes_[category]
//   Music final = userMusicVolume_ * baseVolumes_[Music]
// Vi vay doi base volume khong lam thay doi slider Settings va nguoc lai.
// De debug khi game dang chay: F8 -> Up/Down chon -> Left/Right +/- 5% ->
// Space nghe mau. Khi chot mix, copy cac gia tri console in ra vao
// kDefaultBaseVolumes trong AudioManager.h (hoac nap chung tu config).

AudioManager::AudioManager() = default;

void AudioManager::loadAssets()
{
    // Background & Traffic streams
    if (bgMusic_.openFromFile("assets/audio/bgm.wav")) {
        bgMusic_.setLooping(true);
        bgMusic_.play();
    }
    if (trafficNoise_.openFromFile("assets/audio/traffic.wav"))
        trafficNoise_.setLooping(true);

    // Gameplay SFX buffers
    if (crashBuf_.loadFromFile("assets/audio/crash.mp3")) {
        crashSound_.emplace(crashBuf_);
        environmentPreviewSound_.emplace(crashBuf_);
    }
    if (catBuf_.loadFromFile("assets/audio/cat.wav")) catSound_.emplace(catBuf_);
    if (deerBuf_.loadFromFile("assets/audio/deer.mp3")) deerSound_.emplace(deerBuf_);

    // UI SFX buffers (The ones we just added!)
    if (clickBuf_.loadFromFile("assets/audio/UI.wav")) {
        clickSound_.emplace(clickBuf_);
    }
    if (hoverBuf_.loadFromFile("assets/audio/UI.wav")) {
        hoverSound_.emplace(hoverBuf_);
    }

    applyVolumes();
}

void AudioManager::setUserVolumes(float musicPercent, float sfxPercent)
{
    userMusicVolume_ = std::clamp(musicPercent, 0.f, 100.f);
    userSfxVolume_ = std::clamp(sfxPercent, 0.f, 100.f);
    applyVolumes();
}

void AudioManager::setBaseVolume(AudioCategory category, float multiplier)
{
    baseVolumes_[index(category)] = std::clamp(multiplier, 0.f, 1.f);
    applyVolumes();
    std::cout << "[Audio Mixer] " << displayName(category) << " = "
              << static_cast<int>(baseVolumes_[index(category)] * 100.f + 0.5f) << "%\n";
    logBaseVolumes();
}

void AudioManager::adjustBaseVolume(AudioCategory category, float delta)
{
    setBaseVolume(category, baseVolume(category) + delta);
}

float AudioManager::baseVolume(AudioCategory category) const { return baseVolumes_[index(category)]; }

void AudioManager::startVehicleAmbience()
{
    if (trafficNoise_.getStatus() != sf::SoundSource::Status::Playing) trafficNoise_.play();
}

void AudioManager::pauseVehicleAmbience() { trafficNoise_.pause(); }
bool AudioManager::vehicleAmbiencePlaying() const { return trafficNoise_.getStatus() == sf::SoundSource::Status::Playing; }
void AudioManager::playAnimalSample(bool deer) { if (deer ? deerSound_ : catSound_) (deer ? deerSound_ : catSound_)->play(); }
void AudioManager::playUiCrash() { if (crashSound_) crashSound_->play(); }
void AudioManager::playUiClick() { if (clickSound_) clickSound_->play(); }
void AudioManager::playUiHover() { if (hoverSound_) hoverSound_->play(); }

void AudioManager::playPreview(AudioCategory category)
{
    switch (category) {
    case AudioCategory::Music: bgMusic_.play(); break;
    case AudioCategory::VehicleSFX: startVehicleAmbience(); break;
    case AudioCategory::AnimalSFX: playAnimalSample(); break;
    // Hien tai project chua co asset Environment rieng; dung ban sao crash
    // lam am mau. Ban sao nay van nhan dung EnvironmentSFX base volume.
    // Khi them asset moi, thay bang am thanh nap cong/thoi tiet thuc te.
    case AudioCategory::EnvironmentSFX:
        if (environmentPreviewSound_) environmentPreviewSound_->play();
        break;
    case AudioCategory::UISFX: playUiCrash(); break;
    default: break;
    }
}

const char* AudioManager::displayName(AudioCategory category)
{
    switch (category) {
    case AudioCategory::Music: return "Music";
    case AudioCategory::VehicleSFX: return "Vehicles";
    case AudioCategory::AnimalSFX: return "Animals";
    case AudioCategory::EnvironmentSFX: return "Environment";
    case AudioCategory::UISFX: return "UI";
    default: return "Unknown";
    }
}

void AudioManager::applyVolumes()
{
    bgMusic_.setVolume(userMusicVolume_ * baseVolume(AudioCategory::Music));
    trafficNoise_.setVolume(userSfxVolume_ * baseVolume(AudioCategory::VehicleSFX));
    if (catSound_) catSound_->setVolume(userSfxVolume_ * baseVolume(AudioCategory::AnimalSFX));
    if (deerSound_) deerSound_->setVolume(userSfxVolume_ * baseVolume(AudioCategory::AnimalSFX));
    // crash hien duoc game dung cho thua/va cham, nen thuoc UISFX.
    if (crashSound_) crashSound_->setVolume(userSfxVolume_ * baseVolume(AudioCategory::UISFX));
    if (environmentPreviewSound_)
        environmentPreviewSound_->setVolume(userSfxVolume_ * baseVolume(AudioCategory::EnvironmentSFX));
    if (clickSound_) clickSound_->setVolume(userSfxVolume_ * baseVolume(AudioCategory::UISFX));
    if (hoverSound_) hoverSound_->setVolume(userSfxVolume_ * baseVolume(AudioCategory::UISFX));
}

void AudioManager::logBaseVolumes() const
{
    std::cout << "[Audio Mixer] Base: Music=" << baseVolume(AudioCategory::Music) * 100.f
              << "% Vehicles=" << baseVolume(AudioCategory::VehicleSFX) * 100.f
              << "% Animals=" << baseVolume(AudioCategory::AnimalSFX) * 100.f
              << "% Environment=" << baseVolume(AudioCategory::EnvironmentSFX) * 100.f
              << "% UI=" << baseVolume(AudioCategory::UISFX) * 100.f << "%\n";

}
