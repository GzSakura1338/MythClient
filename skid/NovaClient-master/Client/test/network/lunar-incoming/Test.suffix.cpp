void Pump() {
    // Match the real Minecraft bytecode: the task monitor stays locked while
    // FutureTask.run invokes vanilla or native packet handlers.
    std::lock_guard lock(scheduledMutex);
    JNIEnv env;
    while (!scheduled.empty()) {
        auto task = scheduled.front(); scheduled.pop_front();
        if (!ClientPacketTaskHook(&env, 0, task))
            env.CallVoidMethod(task->packet, h.ProcessPacket, task->listener);
    }
}
int main() {
    JNIEnv env;
    Object first{0, 1}, spawn{0, 2}, velocity{0, 3};
    {
        std::unique_lock queueLock(scheduledMutex);
        const int prior = scheduleAttempts;
        auto netty = std::async(std::launch::async, [&] {
            JNIEnv net;
            PacketInHook(&net, 0, &first, &listener);
        });
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (scheduleAttempts == prior && std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();
        Check(scheduleAttempts != prior, "Netty failed to reach scheduler");
        Check(netty.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout,
              "test did not hold the vanilla task queue monitor");
        Check(moduleMutex.try_lock(), "module/task queue lock inversion");
        moduleMutex.unlock();
        Check(!incomingTasks.Take(&env, &spawn), "tag lock held while entering Java scheduler");
        env.CallVoidMethod(&velocity, h.ProcessPacket, &listener);
        queueLock.unlock(); netty.get();
    }
    Check(callbacks == 0, "receive callback ran before client task");
    delivered.clear(); holdFirst = true;
    std::thread second([&] { JNIEnv net; PacketInHook(&net, 0, &spawn, &listener); }); second.join();
    Pump();
    Check(delivered == std::vector<int>({1, 2}), "held player info must precede spawn");
    Check(callbacks == 2 && refs == 0, "cancelled packet replayed twice or task reference leaked");
    holdFirst = false;

    // Ordinary vanilla tasks and off-thread module replays must NOT produce a
    // second PacketEvent. Reusing a packet object must not confuse task tags.
    int before = callbacks;
    std::thread replay([&] { JNIEnv net; net.CallVoidMethod(&first, h.ProcessPacket, &listener); });
    replay.join(); Pump();
    Check(callbacks == before && delivered.back() == 1, "replay was intercepted again");
    auto unmarked = env.NewObject(0, 0, &first, &listener);
    Check(!ClientPacketTaskHook(&env, 0, unmarked), "vanilla task was intercepted");

    // Several frames of concurrent receive/dispatch, including correction,
    // disconnect and respawn barriers. Verify FIFO and exactly-once delivery.
    delivered.clear();
    constexpr int count = 12000;
    std::vector<Object> packets(count);
    for (int i = 0; i < count; ++i) packets[i] = {i % 5, i + 10};
    std::atomic<bool> done{false};
    std::thread producer([&] {
        JNIEnv net;
        for (auto& packet : packets) PacketInHook(&net, 0, &packet, &listener);
        done = true;
    });
    while (!done) { Pump(); std::this_thread::yield(); }
    producer.join(); Pump();
    Check(delivered.size() == count && refs == 0, "concurrent receive lost packets/references");
    for (int i = 0; i < count; ++i) Check(delivered[i] == i + 10, "incoming stream reordered");

    std::thread failure([&] {
        JNIEnv net; net.failSchedule = true;
        PacketInHook(&net, 0, &first, &listener);
        Check(net.exception, "scheduler exception was swallowed");
        net.exception = false; net.failSchedule = false; net.failRef = true;
        PacketInHook(&net, 0, &first, &listener);
        Check(net.exception, "JNI allocation exception was swallowed");
    }); failure.join();
    Check(refs == 0, "failed scheduling leaked a task tag");

    std::thread pending([&] { JNIEnv net; PacketInHook(&net, 0, &first, &listener); }); pending.join();
    before = callbacks; incomingTasks.Clear(&env); Pump();
    Check(refs == 0 && callbacks == before && delivered.back() == 1,
          "pending task did not survive native shutdown as vanilla");
    circuit = true;
    std::thread disabled([&] { JNIEnv net; PacketInHook(&net, 0, &spawn, &listener); }); disabled.join();
    Pump();
    Check(callbacks == before && delivered.back() == 2, "open circuit still intercepted packets");
    circuit=false; lunar1710=true; delivered.clear(); before=callbacks;
    const int scheduledBefore=scheduleAttempts;
    PacketInHook(&env,0,&first,&listener);
    PacketInHook(&env,0,&spawn,&listener);
    Check(delivered==std::vector<int>({1,2}) && callbacks==before+2,
          "1.7 client queue must dispatch FIFO exactly once");
    Check(scheduleAttempts==scheduledBefore && refs==0,"1.7 touched the absent PacketThreadUtil task path");
    std::puts("PASS: actual Lunar hooks: queue/module lock boundary, FIFO, cancellation/replay, 12000 concurrent packets, barriers, allocation/scheduler errors and unload");
}
