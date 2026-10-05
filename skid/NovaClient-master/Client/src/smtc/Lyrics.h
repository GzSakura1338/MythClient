#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nova::smtc {
struct LyricWord { double time{}, duration{}; std::size_t offset{}, length{}; };
struct SungSpan { std::size_t offset{}, length{}; double fraction{}; };
struct LyricLine {
    double time{};
    std::string text, translation;
    double duration{};
    std::vector<LyricWord> words; // UTF-8 byte spans, with absolute seconds from YRC.
    double Progress(double position, double fallbackEnd) const {
        const double end = duration > 0 ? time + duration : fallbackEnd;
        return std::isfinite(position) && std::isfinite(end) && end > time ?
            std::clamp((position - time) / (end - time), 0.0, 1.0) : 0;
    }
    // Returns the completed UTF-8 prefix and the fractional word after it.
    SungSpan Sung(double position) const {
        SungSpan result{};
        if (!std::isfinite(position)) return result;
        for (const auto& word : words) {
            if (position < word.time) break;
            result = {word.offset, word.length, word.duration > 0 ? std::clamp((position - word.time) / word.duration, 0.0, 1.0) : 1};
        }
        return result;
    }
};
struct Lyrics {
    std::vector<LyricLine> lines;
    int Index(double seconds) const {
        if (!std::isfinite(seconds)) return -1;
        const auto it = std::upper_bound(lines.begin(), lines.end(), seconds,
            [](double time, const LyricLine& line) { return time < line.time; });
        return it == lines.begin() ? -1 : static_cast<int>(it - lines.begin() - 1);
    }
};
std::shared_ptr<const Lyrics> ParseLyrics(std::string_view original, std::string_view translation = {});
std::shared_ptr<const Lyrics> ParseYrcLyrics(std::string_view original, std::string_view translation = {});
}
