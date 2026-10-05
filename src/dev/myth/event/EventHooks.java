package dev.myth.event;

import dev.myth.Myth;
import dev.myth.event.impl.RenderWorldEvent;
import net.fabricmc.fabric.api.client.rendering.v1.RenderStateDataKey;
import net.fabricmc.fabric.api.client.rendering.v1.level.LevelExtractionEvents;
import net.fabricmc.fabric.api.client.rendering.v1.level.LevelRenderEvents;

public final class EventHooks {
    private static final RenderStateDataKey<Float> PARTIAL_TICK =
            RenderStateDataKey.create(() -> "myth:partial_tick");
    private static boolean initialized;

    private EventHooks() {
    }

    public static void initialize() {
        if (initialized) {
            return;
        }
        initialized = true;
        LevelExtractionEvents.END_EXTRACTION.register(context -> context.levelState().setData(
                PARTIAL_TICK, context.deltaTracker().getGameTimeDeltaPartialTick(false)));
        LevelRenderEvents.COLLECT_SUBMITS.register(context -> {
            context.poseStack().pushPose();
            try {
                Myth.EVENT_MANAGER.call(new RenderWorldEvent(
                        context.levelState().getDataOrDefault(PARTIAL_TICK, 0.0F),
                        context.poseStack(), context));
            } finally {
                context.poseStack().popPose();
            }
        });
    }
}
