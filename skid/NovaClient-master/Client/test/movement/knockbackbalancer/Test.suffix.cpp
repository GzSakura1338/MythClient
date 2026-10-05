using namespace nova;
bool Receive(mock::Packet& p, bool cancelled = false) {
    event::PacketEvent e(event::EventType::Incoming, &p);
    e.SetCancelled(cancelled);
    (void)event::EventBus::Get().Post(e);
    if (!e.IsCancelled()) java::HandleIncomingPacket(&p);
    return e.IsCancelled();
}
void Tick(module::Module& m) {
    m.tick(); event::GameTickEvent e(event::EventType::Pre);
    (void)event::EventBus::Get().Post(e); m.tick();
}
void Enable(module::Module& m) {
    mock::failRef = false; mock::world = mock::player = 1; mock::played.clear();
    m.setEnabled(true); Tick(m); assert(m.isEnabled() && mock::wanted == 1);
}
void Disable(module::Module& m) { m.setEnabled(false); Tick(m); }
void Played(std::initializer_list<int> expected) { assert(mock::played == std::vector<int>(expected)); }
int main() {
    auto m = module::MakeKnockBackBalancerModule(); event::EventBus::Get().Seal();
    mock::Packet first{1,1}, between{2}, remote{3,1,2}, second{4,1}, tail{5}, third{6,1}, after{7};
    Enable(*m);
    assert(!Receive(between)); assert(!Receive(remote)); mock::played.clear();
    // Zero/negative/vertical own velocity qualifies, exactly as Naven's ID test.
    assert(Receive(first)); assert(Receive(between)); assert(Receive(remote));
    assert(Receive(second)); assert(Receive(tail)); assert(Receive(third)); assert(Receive(after));
    assert(mock::refs == 7 && m->suffix() == "3 KB");
    event::PacketEvent outbound(event::EventType::Outgoing, &between);
    assert(!event::EventBus::Get().Post(outbound));
    for (int i = 0; i < 200; ++i) Tick(*m);
    Played({}); assert(m->isEnabled() && mock::refs == 7); // No automatic/partial release.
    Disable(*m); Played({1,2,3,4,5,6,7}); assert(!m->isEnabled() && mock::refs == 0 && mock::wanted == 0);
    assert(!Receive(first)); Played({1,2,3,4,5,6,7,1});
    Enable(*m); Receive(first); assert(m->suffix() == "1 KB"); Disable(*m); Played({1});

    mock::Packet correction{8,2};
    Enable(*m); assert(!Receive(correction)); mock::played.clear();
    Receive(first); Receive(between); assert(Receive(correction)); Played({});
    Tick(*m); Played({}); assert(m->isEnabled() && mock::refs == 3);
    Receive(second); Tick(*m); Played({}); assert(m->suffix() == "2 KB");
    Disable(*m); Played({1,2,8,4}); assert(mock::refs == 0); // Correction stays in FIFO until disabled.

    for (int kind : {3,4}) {
        mock::Packet boundary{9,kind}; Enable(*m); Receive(first); Receive(between);
        assert(!Receive(boundary)); Tick(*m); Played({9}); assert(mock::refs == 0 && !m->isEnabled());
    }
    Enable(*m); Receive(first); mock::world = 2; Tick(*m); Played({}); assert(mock::refs == 0);
    Enable(*m); Receive(first); mock::player = 2; Disable(*m); Played({}); assert(mock::refs == 0);

    Enable(*m); assert(Receive(first, true)); assert(mock::refs == 0 && m->suffix() == "Waiting");
    Receive(first); Receive(between, true); Tick(*m); Played({}); assert(m->isEnabled() && mock::refs == 1);
    Receive(second); Tick(*m); Played({}); assert(m->suffix() == "2 KB");
    Disable(*m); Played({1,4}); assert(mock::refs == 0);
    Enable(*m); Receive(first); mock::failRef = true; assert(!Receive(between)); Tick(*m);
    Played({1,2}); assert(mock::refs == 0 && !m->isEnabled());

    Enable(*m); Receive(first); Receive(second);
    mock::duringReplay = [&] { assert(Receive(third)); assert(Receive(after)); };
    Disable(*m); Played({1,4,6,7}); assert(mock::refs == 0 && mock::wanted == 0);

    Enable(*m); Disable(*m); assert(!m->isEnabled() && mock::wanted == 0);
    Enable(*m);
    for (int i = 0; i < 10000; ++i) {
        assert(Receive(first)); assert(Receive(between));
        if (i % 10 == 0) { Tick(*m); Played({}); assert(m->isEnabled()); }
    }
    Tick(*m); Played({}); assert(m->isEnabled() && mock::refs == 20000 && m->suffix() == "10000 KB");
    Disable(*m); assert(!m->isEnabled() && mock::refs == 0 && mock::played.size() == 20000);
    for (std::size_t i = 0; i < mock::played.size(); i += 2) { assert(mock::played[i] == 1); assert(mock::played[i+1] == 2); }
    Enable(*m); Receive(first); m.reset(); assert(mock::refs == 0 && mock::wanted == 0);
    puts("PASS: 10000 KB / 20000 packets held until disable, correction and cancellation retention, FIFO, lifecycle and cleanup");
}
