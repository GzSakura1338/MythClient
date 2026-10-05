package net.minecraft.client.renderer.feature;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.renderer.feature.submit.SubmitNode;

@Environment(EnvType.CLIENT)
public interface FeatureRenderer<Submit extends SubmitNode> extends AutoCloseable {
	default void beginPrepare(final FeatureFrameContext context) {
	}

	void prepareGroup(FeatureFrameContext context, List<Submit> submits, boolean strictlyOrdered);

	default void finishPrepare(final FeatureFrameContext context) {
	}

	void executeGroup(FeatureFrameContext context, int groupIndex, List<Submit> submits, boolean strictlyOrdered);

	default void finishExecute(final FeatureFrameContext context) {
	}

	@Override
	default void close() {
	}
}
