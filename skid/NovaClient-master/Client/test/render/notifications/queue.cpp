#include <cassert>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "ui/notifications/Notifications.h"

int main() {
    using namespace nova::notifications;
    Post("");
    assert(Drain().empty());
    Post("Config\nloaded", Type::Success, Source::Config);
    auto messages = Drain();
    assert(messages.size() == 1 && messages[0].text == "Config loaded");
    assert(messages[0].type == Type::Success && messages[0].source == Source::Config);
    assert(Drain().empty());

    for (int i = 0; i < 40; ++i) Post(std::to_string(i));
    messages = Drain();
    assert(messages.size() == 32 && messages.front().text == "8" && messages.back().text == "39");

    const std::string codepoint = "\xE9\x80\x9A";
    std::string longText;
    for (int i = 0; i < 200; ++i) longText += codepoint;
    Post(longText);
    messages = Drain();
    assert(messages.size() == 1 && messages[0].text.size() == 513);
    assert(messages[0].text == longText.substr(0, 510) + "...");

    std::vector<std::thread> producers;
    for (int thread = 0; thread < 4; ++thread)
        producers.emplace_back([thread] {
            for (int i = 0; i < 200; ++i) Post(std::to_string(thread) + ":" + std::to_string(i));
        });
    for (auto& producer : producers) producer.join();
    messages = Drain();
    assert(messages.size() == 32);
    for (const auto& message : messages) assert(message.text.find(':') != std::string::npos);
    assert(Drain().empty());
    std::cout << "Notification queue: FIFO, bounds, UTF-8 and concurrent producers passed\n";
}
