#pragma once

#include <string>
#include <vector>

namespace nova::notifications {

enum class Type { Info, Success, Warning, Error };
enum class Source { General, Module, Config };

struct Message {
    std::string text;
    Type type = Type::Info;
    Source source = Source::General;
};

// Safe for producers on game/network threads; rendering stays on the UI thread.
void Post(std::string text, Type type = Type::Info, Source source = Source::General);
std::vector<Message> Drain();

}
