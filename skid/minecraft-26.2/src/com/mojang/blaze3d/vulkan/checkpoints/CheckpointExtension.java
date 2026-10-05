package com.mojang.blaze3d.vulkan.checkpoints;

import com.mojang.blaze3d.vulkan.VulkanDevice;
import com.mojang.blaze3d.vulkan.VulkanQueue;
import java.util.List;
import java.util.function.Supplier;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import org.lwjgl.vulkan.VkCommandBuffer;

@Environment(EnvType.CLIENT)
public interface CheckpointExtension extends AutoCloseable {
	CheckpointExtension.CheckpointStorage createStorage(VulkanDevice device, VulkanQueue queue, int maxFramesInFlight);

	List<CheckpointExtension.QueueCheckpoints> retrieveCheckpoints(boolean isDeviceLost);

	@Override
	void close();

	@Environment(EnvType.CLIENT)
	interface CheckpointStorage {
		void rotate();

		void recordCheckpoint(VkCommandBuffer commandBuffer, CheckpointExtension.CheckpointType type, Supplier<String> label);
	}

	@Environment(EnvType.CLIENT)
	enum CheckpointType {
		BEGIN_RENDER_PASS,
		END_RENDER_PASS;
	}

	@Environment(EnvType.CLIENT)
	record QueueCheckpoints(long queue, List<CheckpointExtension.StageCheckpoint> checkpoints) {
	}

	@Environment(EnvType.CLIENT)
	record StageCheckpoint(long stage, CheckpointExtension.CheckpointType type, String label) {
	}
}
