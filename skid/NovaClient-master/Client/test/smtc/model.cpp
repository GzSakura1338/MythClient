#include "smtc/MediaSnapshot.h"
#include "smtc/MediaCommand.h"
#include "smtc/backend/TimelineSample.h"
#include <cassert>
#include <limits>
#include <cstdio>
int main() {
    using namespace nova::smtc;
    MediaSnapshot data;
    data.start = 10; data.end = 110; data.position = 20; data.playing = true; data.rate = 2;
    const auto later = data.sampled + std::chrono::seconds(5);
    assert(data.Position(later) == 30);
    assert(data.Position(data.sampled - std::chrono::seconds(5)) == 20);
    assert(data.Position(data.sampled + std::chrono::seconds(500)) == 110);
    data.playing = false; assert(data.Position(later) == 20);
    data.position = -10; assert(data.Position(later) == 10);
    data.position = std::numeric_limits<double>::quiet_NaN(); assert(data.Position(later) == 10);
    data.end = data.start; assert(!data.HasDuration());
    data.end = std::numeric_limits<double>::infinity(); assert(!data.HasDuration());
    backend::TimelineGate gate;
    backend::TimelineSample sample{1, 0x100000, "123_ABC", "123", 50, 120};
    const auto time = Clock::now();
    assert(!gate.Accept(sample, time));
    assert(!gate.Accept(sample, time + std::chrono::milliseconds(999)));
    assert(gate.Accept(sample, time + std::chrono::seconds(1)));
    sample.generation = 2; // A restarted track cannot reuse a previous accepted timeline.
    assert(!gate.Accept(sample, time + std::chrono::seconds(2)));
    sample.position = std::numeric_limits<double>::quiet_NaN();
    assert(!gate.Accept(sample, time + std::chrono::seconds(3)));
    sample.position = 121; assert(!sample.Valid());
    sample.position = 50; sample.playId = "456_ABC"; assert(!sample.Valid());
    sample.playId = "123_ABC"; sample.duration = 0; assert(!sample.Valid());
    data.end = 120; data.start = 0; data.position = 50; data.rate = 0; data.playing = true;
    assert(data.Position(later) == 50); // A stalled native feed never fabricates advancement.
    auto lyrics = ParseLyrics("[ar:ignored]\n[00:02.5]Second\n[00:01.00][00:04.000]First\n[00:03.00]\n[offset:500]\n",
        "[00:00.50]Translation\n");
    assert(lyrics->lines.size() == 4 && lyrics->lines[0].time == .5);
    assert(lyrics->lines[0].translation == "Translation");
    assert(lyrics->Index(.49) == -1 && lyrics->Index(.5) == 0 && lyrics->Index(2.0) == 1);
    assert(lyrics->lines[lyrics->Index(2.5)].text.empty()); // Instrumental gap clears the old line.
    assert(lyrics->Index(3.5) == 3 && lyrics->Index(.5) == 0); // Seeking backwards is stateless.
    assert(lyrics->Index(std::numeric_limits<double>::infinity()) == -1);
    assert(ParseLyrics("[00:99]bad\n[NaN]bad\n[00:01.1234]bad")->lines.empty());
    assert(ParseLyrics(std::string(1024 * 1024 + 1, 'x'))->lines.empty());
    assert(ParseLyrics("[00:01]one\n[00:01]two")->lines[0].text == "one / two");
    const auto timed = ParseYrcLyrics("{\"t\":0,\"c\":[]}\n[1000,3000](1000,500,0)你(2000,1000,0)好(3000,1000,0)!\n",
        "[00:01.000]Hello");
    assert(timed->lines.size() == 1);
    const auto& line = timed->lines[0];
    assert(line.text == "你好!" && line.translation == "Hello" && line.duration == 3 && line.words.size() == 3);
    assert(line.Sung(.9).fraction == 0);
    assert(line.Sung(1.25).offset == 0 && line.Sung(1.25).length == 3 && line.Sung(1.25).fraction == .5);
    assert(line.Sung(1.75).fraction == 1 && line.Sung(1.75).offset == 0); // Word gap holds its highlight.
    assert(line.Sung(2.5).offset == 3 && line.Sung(2.5).fraction == .5);
    assert(line.Sung(5).offset == 6 && line.Sung(5).fraction == 1);
    assert(line.Sung(1.25).fraction == .5); // Seek back restores partial word progress.
    assert(line.Progress(2.5, 100) == .5 && line.Progress(10, 100) == 1);
    assert(ParseYrcLyrics("[1000,1000](500,100,0)bad")->lines.empty());
    assert(ParseYrcLyrics("[1000,1000](1000,2000,0)bad")->lines.empty());
    assert(ParseYrcLyrics("[1000,1000](1000,0,0)instant")->lines[0].Sung(1).fraction == 1);
    assert(ParseYrcLyrics("[1000,1000](1000,1000,0)word (aside)")->lines[0].text == "word (aside)");
    std::deque<MediaCommand> queue;
    assert(!QueueCommand(queue, {Action::Seek, 0, 1, 10}));
    assert(!QueueCommand(queue, {Action::Seek, 1, 1, std::numeric_limits<double>::infinity()}));
    assert(QueueCommand(queue, {Action::Seek, 1, 1, 10}));
    assert(QueueCommand(queue, {Action::Seek, 1, 1, 20}));
    assert(queue.size() == 1 && queue.back().position == 20);
    assert(QueueCommand(queue, {Action::Seek, 1, 2, 30}));
    assert(queue.size() == 2);
    while (queue.size() < 16) assert(QueueCommand(queue, {Action::Toggle, 1, 2}));
    assert(!QueueCommand(queue, {Action::Next, 1, 2}));
    puts("PASS: timeline rates, pause, nonzero origin, bounds, unknown duration and bounded/coalesced commands");
}
