#pragma once
#include <memory>
namespace nova::ui::cswin {
enum class Track { Inhuman, EZ4ENCE };
// Owns its waveOut device; stopping this cue never stops game/SMTC audio.
class Audio {
public:
    Audio();
    ~Audio();
    Audio(const Audio&)=delete;
    Audio& operator=(const Audio&)=delete;
    bool Play(Track track=Track::Inhuman);
    double DurationSeconds() const;
    void Stop();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
