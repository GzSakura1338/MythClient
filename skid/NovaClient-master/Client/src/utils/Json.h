#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nova::json {

class Value {
public:
    enum class Kind : int {
        Null = 0,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    Value() = default;
    Value(std::nullptr_t) {}
    Value(bool value) : kind_(Kind::Bool), boolean_(value) {}
    Value(double value) : kind_(Kind::Number), number_(value) {}
    Value(int value) : Value(static_cast<double>(value)) {}
    Value(std::string value) : kind_(Kind::String), text_(std::move(value)) {}
    Value(const char* value) : Value(std::string(value)) {}

    static Value Object();
    static Value Array();

    [[nodiscard]] Kind kind() const { return kind_; }
    [[nodiscard]] bool isNull() const { return kind_ == Kind::Null; }
    [[nodiscard]] bool isBool() const { return kind_ == Kind::Bool; }
    [[nodiscard]] bool isNumber() const { return kind_ == Kind::Number; }
    [[nodiscard]] bool isString() const { return kind_ == Kind::String; }
    [[nodiscard]] bool isArray() const { return kind_ == Kind::Array; }
    [[nodiscard]] bool isObject() const { return kind_ == Kind::Object; }

    [[nodiscard]] bool asBool(bool fallback) const;
    [[nodiscard]] double asNumber(double fallback) const;
    [[nodiscard]] std::string asString(std::string_view fallback = {}) const;

    [[nodiscard]] const Value* Find(std::string_view key) const;
    [[nodiscard]] const std::vector<Value>& Items() const { return array_; }
    [[nodiscard]] std::size_t Size() const;

    Value& operator[](std::string_view key);
    void Push(Value value);

    [[nodiscard]] std::string Dump(int indent = 2) const;

private:
    void Write(std::string& out, int indent, int depth) const;

    Kind kind_ = Kind::Null;
    bool boolean_ = false;
    double number_ = 0.0;
    std::string text_;
    std::vector<Value> array_;
    std::vector<std::pair<std::string, Value>> object_;
};

[[nodiscard]] std::optional<Value> Parse(std::string_view text);

}
