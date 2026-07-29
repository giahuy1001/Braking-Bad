#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <optional>

// Cac kenh dung de can bang asset khi debug. Day KHONG phai la thanh
// am luong nguoi dung: base volume la he so 0.0 - 1.0 cua lap trinh vien.
enum class AudioCategory : std::size_t
{
    Music,
    VehicleSFX,
    AnimalSFX,
    EnvironmentSFX,
    UISFX,
    Count
};

// Quan ly am thanh va Debug Audio Mixer.
//
// Cong thuc (SFML dung don vi phan tram):
//   Final_Volume = User_SFX_Volume * Group_Base_Volume
//   Final_Music_Volume = User_Music_Volume * Music_Base_Volume
// Vi du User SFX = 80 (%) va AnimalSFX = 0.75 thi tieng dong vat phat
// o 60 (%). Base volume chi dung de can bang do lon khong dong deu cua asset.
//
// Debug trong game: UIManager mo mixer bang F8; Up/Down chon kenh,
// Left/Right giam/tang 5%, Space phat mau. Moi thay doi duoc in ra console.
// Sau khi tim duoc gia tri phu hop, hay copy cac so trong kDefaultBaseVolumes
// vao day (hoac doc chung tu file cau hinh) de "hardcode" mix chinh thuc,
// roi co the tat/bo UI debug o ban release.
class AudioManager
{
public:
    AudioManager();

    void loadAssets();
    void setUserVolumes(float musicPercent, float sfxPercent);
    void setBaseVolume(AudioCategory category, float multiplier);
    void adjustBaseVolume(AudioCategory category, float delta);
    float baseVolume(AudioCategory category) const;

    void startVehicleAmbience();
    void pauseVehicleAmbience();
    bool vehicleAmbiencePlaying() const;
    void playAnimalSample(bool deer = false);
    void playUiCrash();
    void playPreview(AudioCategory category);

    static const char* displayName(AudioCategory category);

private:
    static constexpr std::size_t index(AudioCategory category)
    {
        return static_cast<std::size_t>(category);
    }

    // Noi de hardcode ket qua mix da chot sau khi debug.
    static constexpr std::array<float, static_cast<std::size_t>(AudioCategory::Count)>
        kDefaultBaseVolumes = { 1.00f, 0.65f, 0.15f, 1.00f, 0.30f };

    void applyVolumes();
    void logBaseVolumes() const;

    float userMusicVolume_ = 100.f;
    float userSfxVolume_ = 100.f;
    std::array<float, static_cast<std::size_t>(AudioCategory::Count)> baseVolumes_
        = kDefaultBaseVolumes;

    sf::Music bgMusic_;
    sf::Music trafficNoise_;
    sf::SoundBuffer crashBuf_, catBuf_, deerBuf_;
    std::optional<sf::Sound> crashSound_, catSound_, deerSound_;

    // Environment hien chua co asset rieng. Ban sao nay cho phep dung crash.mp3
    // lam preview tam thoi ma khong lam thay doi volume cua UISFX/crash that.
    std::optional<sf::Sound> environmentPreviewSound_;
};
