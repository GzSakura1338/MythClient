#include "smtc/Lyrics.h"
#include <charconv>

namespace nova::smtc {
namespace {
bool Number(std::string_view text, int& value) {
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
bool Timestamp(std::string_view tag, double& time) {
    const auto colon = tag.find(':');
    if (colon == std::string_view::npos) return false;
    int minutes{}, seconds{}, fraction{};
    const auto tail = tag.substr(colon + 1); const auto dot = tail.find('.');
    if (!Number(tag.substr(0, colon), minutes) || minutes < 0 || minutes > 1440 ||
        !Number(tail.substr(0, dot), seconds) || seconds < 0 || seconds >= 60) return false;
    double scale = 1;
    if (dot != std::string_view::npos) {
        const auto part = tail.substr(dot + 1);
        if (part.empty() || part.size() > 3 || !Number(part, fraction) || fraction < 0) return false;
        for (std::size_t i = 0; i < part.size(); ++i) scale *= 10;
    }
    time = minutes * 60 + seconds + fraction / scale; return true;
}
std::vector<LyricLine> Parse(std::string_view lrc) {
    std::vector<LyricLine> result;
    if (lrc.size() > 1024 * 1024) return result;
    int offset = 0;
    if (lrc.starts_with("\xEF\xBB\xBF")) lrc.remove_prefix(3);
    while (!lrc.empty() && result.size() < 4096) {
        const auto end = lrc.find('\n'); auto line = lrc.substr(0, end);
        lrc = end == std::string_view::npos ? std::string_view{} : lrc.substr(end + 1);
        std::vector<double> times;
        while (line.starts_with('[')) {
            const auto close = line.find(']'); if (close == std::string_view::npos) break;
            const auto tag = line.substr(1, close - 1); double time{};
            if (Timestamp(tag, time) && times.size() < 32) times.push_back(time);
            else if (tag.starts_with("offset:")) {
                int value{}; auto number = tag.substr(7); if (number.starts_with('+')) number.remove_prefix(1);
                if (Number(number, value) && std::abs(static_cast<double>(value)) <= 3600000) offset = value;
            }
            line.remove_prefix(close + 1);
        }
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.remove_suffix(1);
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.remove_prefix(1);
        if (line.size() > 2048) continue;
        std::string text(line);
        for (auto& c : text) if (static_cast<unsigned char>(c) < 32) c = ' ';
        for (const auto time : times) if (result.size() < 4096) result.push_back({time, text, {}});
    }
    for (auto& line : result) line.time = std::max(0.0, line.time - offset / 1000.0);
    std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
    std::vector<LyricLine> merged;
    for (auto& line : result) {
        if (!merged.empty() && merged.back().time == line.time) {
            if (!line.text.empty() && merged.back().text != line.text && merged.back().text.size() + line.text.size() < 2048) {
                if (!merged.back().text.empty()) merged.back().text += " / ";
                merged.back().text += line.text;
            }
        } else merged.push_back(std::move(line));
    }
    return merged;
}
}
std::shared_ptr<const Lyrics> ParseLyrics(std::string_view original, std::string_view translation) {
    auto lyrics = std::make_shared<Lyrics>(); lyrics->lines = Parse(original);
    const auto translated = Parse(translation);
    for (auto& line : lyrics->lines) {
        auto it = std::lower_bound(translated.begin(), translated.end(), line.time - .15,
            [](const LyricLine& value, double time) { return value.time < time; });
        if (it != translated.end() && std::abs(it->time - line.time) <= .15 && it->text != line.text) line.translation = it->text;
    }
    return lyrics;
}
std::shared_ptr<const Lyrics> ParseYrcLyrics(std::string_view original, std::string_view translation) {
    auto lyrics = std::make_shared<Lyrics>();
    if (original.size() > 1024 * 1024) return lyrics;
    auto pair = [](std::string_view value, int& start, int& duration) {
        const auto comma = value.find(',');
        return comma != std::string_view::npos && Number(value.substr(0, comma), start) &&
            Number(value.substr(comma + 1), duration) && start >= 0 && start <= 86400000 && duration >= 0 && duration <= 86400000 - start;
    };
    auto wordTag = [&](std::string_view line, std::size_t at, int& start, int& duration, std::size_t& end) {
        end = line.find(')', at); if (end == std::string_view::npos) return false;
        const auto value = line.substr(at + 1, end - at - 1);
        const auto comma = value.rfind(','); int flag{};
        return comma != std::string_view::npos && Number(value.substr(comma + 1), flag) && pair(value.substr(0, comma), start, duration);
    };
    while (!original.empty() && lyrics->lines.size() < 4096) {
        const auto end = original.find('\n'); auto row = original.substr(0, end);
        original = end == std::string_view::npos ? std::string_view{} : original.substr(end + 1);
        while (!row.empty() && (row.back() == '\r' || row.back() == ' ')) row.remove_suffix(1);
        while (row.starts_with(' ') || row.starts_with('\t')) row.remove_prefix(1);
        if (!row.starts_with('[')) continue; // Native YRC also carries JSON contributor metadata.
        const auto close = row.find(']'); int start{}, duration{};
        if (close == std::string_view::npos || !pair(row.substr(1, close - 1), start, duration) || duration == 0) return std::make_shared<Lyrics>();
        row.remove_prefix(close + 1);
        LyricLine line{start / 1000.0, {}, {}, duration / 1000.0};
        std::size_t at = 0;
        while (at < row.size()) {
            int wordStart{}, wordDuration{}; std::size_t tagEnd{};
            if (row[at] != '(' || !wordTag(row, at, wordStart, wordDuration, tagEnd)) return std::make_shared<Lyrics>();
            std::size_t next = row.find('(', tagEnd + 1);
            while (next != std::string_view::npos) {
                int a{}, b{}; std::size_t e{};
                if (wordTag(row, next, a, b, e)) break;
                next = row.find('(', next + 1);
            }
            const auto text = row.substr(tagEnd + 1, next == std::string_view::npos ? row.size() - tagEnd - 1 : next - tagEnd - 1);
            if (wordStart < start || wordStart + wordDuration > start + duration ||
                (!line.words.empty() && wordStart / 1000.0 < line.words.back().time) ||
                line.text.size() + text.size() > 2048 || line.words.size() >= 512) return std::make_shared<Lyrics>();
            line.words.push_back({wordStart / 1000.0, wordDuration / 1000.0, line.text.size(), text.size()});
            line.text.append(text);
            at = next == std::string_view::npos ? row.size() : next;
        }
        if (!line.words.empty() && !line.text.empty()) lyrics->lines.push_back(std::move(line));
    }
    std::stable_sort(lyrics->lines.begin(), lyrics->lines.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
    const auto translated = Parse(translation);
    for (auto& line : lyrics->lines) {
        const auto it = std::lower_bound(translated.begin(), translated.end(), line.time - .15,
            [](const LyricLine& value, double time) { return value.time < time; });
        if (it != translated.end() && std::abs(it->time - line.time) <= .15 && it->text != line.text) line.translation = it->text;
    }
    return lyrics;
}
}
