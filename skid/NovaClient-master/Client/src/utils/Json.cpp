#include "utils/Json.h"

#include <charconv>
#include <cmath>
#include <cstdio>

namespace nova::json {
namespace {

constexpr int kMaxDepth = 64;

void AppendUtf8(std::string& out, unsigned int code) {
    if (code <= 0x7Fu) {
        out.push_back(static_cast<char>(code));
    } else if (code <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | (code >> 6)));
        out.push_back(static_cast<char>(0x80u | (code & 0x3Fu)));
    } else if (code <= 0xFFFFu) {
        out.push_back(static_cast<char>(0xE0u | (code >> 12)));
        out.push_back(static_cast<char>(0x80u | ((code >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (code & 0x3Fu)));
    } else {
        out.push_back(static_cast<char>(0xF0u | (code >> 18)));
        out.push_back(static_cast<char>(0x80u | ((code >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((code >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (code & 0x3Fu)));
    }
}

void EscapeInto(std::string& out, std::string_view text) {
    out.push_back('"');
    for (const char raw : text) {
        const auto byte = static_cast<unsigned char>(raw);
        switch (raw) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (byte < 0x20u) {
                char escaped[8];
                std::snprintf(escaped, sizeof(escaped), "\\u%04x", byte);
                out += escaped;
            } else {
                out.push_back(raw);
            }
        }
    }
    out.push_back('"');
}

class Reader {
public:
    explicit Reader(std::string_view text) : text_(text) {}

    std::optional<Value> Run() {
        SkipSpace();
        std::optional<Value> value = ParseValue(0);
        if (!value)
            return std::nullopt;
        SkipSpace();
        if (at_ != text_.size())
            return std::nullopt;
        return value;
    }

private:
    void SkipSpace() {
        while (at_ < text_.size()) {
            const char c = text_[at_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                ++at_;
            else
                break;
        }
    }

    bool Take(char expected) {
        if (at_ < text_.size() && text_[at_] == expected) {
            ++at_;
            return true;
        }
        return false;
    }

    bool TakeWord(std::string_view word) {
        if (text_.compare(at_, word.size(), word) == 0) {
            at_ += word.size();
            return true;
        }
        return false;
    }

    std::optional<Value> ParseValue(int depth) {
        if (depth > kMaxDepth)
            return std::nullopt;
        SkipSpace();
        if (at_ >= text_.size())
            return std::nullopt;

        switch (text_[at_]) {
        case '{': return ParseObject(depth);
        case '[': return ParseArray(depth);
        case '"': {
            std::optional<std::string> text = ParseString();
            if (!text)
                return std::nullopt;
            return Value(std::move(*text));
        }
        case 't':
            if (TakeWord("true"))
                return Value(true);
            return std::nullopt;
        case 'f':
            if (TakeWord("false"))
                return Value(false);
            return std::nullopt;
        case 'n':
            if (TakeWord("null"))
                return Value(nullptr);
            return std::nullopt;
        default:
            return ParseNumber();
        }
    }

    std::optional<Value> ParseObject(int depth) {
        if (!Take('{'))
            return std::nullopt;

        Value object = Value::Object();
        SkipSpace();
        if (Take('}'))
            return object;

        while (true) {
            SkipSpace();
            std::optional<std::string> key = ParseString();
            if (!key)
                return std::nullopt;
            SkipSpace();
            if (!Take(':'))
                return std::nullopt;

            std::optional<Value> value = ParseValue(depth + 1);
            if (!value)
                return std::nullopt;
            object[std::move(*key)] = std::move(*value);

            SkipSpace();
            if (Take(','))
                continue;
            if (Take('}'))
                return object;
            return std::nullopt;
        }
    }

    std::optional<Value> ParseArray(int depth) {
        if (!Take('['))
            return std::nullopt;

        Value array = Value::Array();
        SkipSpace();
        if (Take(']'))
            return array;

        while (true) {
            std::optional<Value> value = ParseValue(depth + 1);
            if (!value)
                return std::nullopt;
            array.Push(std::move(*value));

            SkipSpace();
            if (Take(','))
                continue;
            if (Take(']'))
                return array;
            return std::nullopt;
        }
    }

    std::optional<std::string> ParseString() {
        if (!Take('"'))
            return std::nullopt;

        std::string out;
        while (at_ < text_.size()) {
            const char c = text_[at_++];
            if (c == '"')
                return out;

            if (c != '\\') {
                if (static_cast<unsigned char>(c) < 0x20)
                    return std::nullopt;
                out.push_back(c);
                continue;
            }

            if (at_ >= text_.size())
                return std::nullopt;

            const char escape = text_[at_++];
            switch (escape) {
            case '"':  out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/':  out.push_back('/'); break;
            case 'b':  out.push_back('\b'); break;
            case 'f':  out.push_back('\f'); break;
            case 'n':  out.push_back('\n'); break;
            case 'r':  out.push_back('\r'); break;
            case 't':  out.push_back('\t'); break;
            case 'u': {
                std::optional<unsigned int> high = ParseHex4();
                if (!high)
                    return std::nullopt;

                unsigned int code = *high;
                if (code >= 0xD800u && code <= 0xDBFFu) {
                    if (!Take('\\') || !Take('u'))
                        return std::nullopt;
                    std::optional<unsigned int> low = ParseHex4();
                    if (!low || *low < 0xDC00u || *low > 0xDFFFu)
                        return std::nullopt;
                    code = 0x10000u + ((code - 0xD800u) << 10) + (*low - 0xDC00u);
                }
                AppendUtf8(out, code);
                break;
            }
            default:
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    std::optional<unsigned int> ParseHex4() {
        if (at_ + 4 > text_.size())
            return std::nullopt;
        unsigned int code = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = text_[at_++];
            code <<= 4;
            if (c >= '0' && c <= '9')
                code |= static_cast<unsigned int>(c - '0');
            else if (c >= 'a' && c <= 'f')
                code |= static_cast<unsigned int>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                code |= static_cast<unsigned int>(c - 'A' + 10);
            else
                return std::nullopt;
        }
        return code;
    }

    std::optional<Value> ParseNumber() {
        const char* begin = text_.data() + at_;
        const char* end = text_.data() + text_.size();

        double parsed = 0.0;
        const std::from_chars_result result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc{} || result.ptr == begin)
            return std::nullopt;
        if (!std::isfinite(parsed))
            return std::nullopt;

        at_ += static_cast<std::size_t>(result.ptr - begin);
        return Value(parsed);
    }

    std::string_view text_;
    std::size_t at_ = 0;
};

}

Value Value::Object() {
    Value value;
    value.kind_ = Kind::Object;
    return value;
}

Value Value::Array() {
    Value value;
    value.kind_ = Kind::Array;
    return value;
}

bool Value::asBool(bool fallback) const {
    return kind_ == Kind::Bool ? boolean_ : fallback;
}

double Value::asNumber(double fallback) const {
    return kind_ == Kind::Number ? number_ : fallback;
}

std::string Value::asString(std::string_view fallback) const {
    return kind_ == Kind::String ? text_ : std::string(fallback);
}

const Value* Value::Find(std::string_view key) const {
    if (kind_ != Kind::Object)
        return nullptr;
    for (const auto& entry : object_) {
        if (entry.first == key)
            return &entry.second;
    }
    return nullptr;
}

std::size_t Value::Size() const {
    if (kind_ == Kind::Array)
        return array_.size();
    if (kind_ == Kind::Object)
        return object_.size();
    return 0;
}

Value& Value::operator[](std::string_view key) {
    if (kind_ != Kind::Object) {
        kind_ = Kind::Object;
        array_.clear();
    }
    for (auto& entry : object_) {
        if (entry.first == key)
            return entry.second;
    }
    object_.emplace_back(std::string(key), Value{});
    return object_.back().second;
}

void Value::Push(Value value) {
    if (kind_ != Kind::Array) {
        kind_ = Kind::Array;
        object_.clear();
    }
    array_.push_back(std::move(value));
}

void Value::Write(std::string& out, int indent, int depth) const {
    const bool pretty = indent > 0;
    const std::string pad(pretty ? static_cast<std::size_t>(indent) * (depth + 1) : 0, ' ');
    const std::string closingPad(pretty ? static_cast<std::size_t>(indent) * depth : 0, ' ');

    switch (kind_) {
    case Kind::Null:
        out += "null";
        break;
    case Kind::Bool:
        out += boolean_ ? "true" : "false";
        break;
    case Kind::Number: {
        if (!std::isfinite(number_)) {
            out += "0";
            break;
        }
        char buffer[32];
        const std::to_chars_result result =
            std::to_chars(buffer, buffer + sizeof(buffer), number_);
        if (result.ec != std::errc{}) {
            out += "0";
            break;
        }
        out.append(buffer, result.ptr);
        break;
    }
    case Kind::String:
        EscapeInto(out, text_);
        break;
    case Kind::Array: {
        if (array_.empty()) {
            out += "[]";
            break;
        }
        out.push_back('[');
        for (std::size_t i = 0; i < array_.size(); ++i) {
            if (i > 0)
                out.push_back(',');
            if (pretty) {
                out.push_back('\n');
                out += pad;
            }
            array_[i].Write(out, indent, depth + 1);
        }
        if (pretty) {
            out.push_back('\n');
            out += closingPad;
        }
        out.push_back(']');
        break;
    }
    case Kind::Object: {
        if (object_.empty()) {
            out += "{}";
            break;
        }
        out.push_back('{');
        for (std::size_t i = 0; i < object_.size(); ++i) {
            if (i > 0)
                out.push_back(',');
            if (pretty) {
                out.push_back('\n');
                out += pad;
            }
            EscapeInto(out, object_[i].first);
            out.push_back(':');
            if (pretty)
                out.push_back(' ');
            object_[i].second.Write(out, indent, depth + 1);
        }
        if (pretty) {
            out.push_back('\n');
            out += closingPad;
        }
        out.push_back('}');
        break;
    }
    }
}

std::string Value::Dump(int indent) const {
    std::string out;
    Write(out, indent, 0);
    out.push_back('\n');
    return out;
}

std::optional<Value> Parse(std::string_view text) {
    return Reader(text).Run();
}

}
