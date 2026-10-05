package net.minecraft.client.renderer.state.level;

import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.fabric.api.client.rendering.v1.FabricRenderState;
import net.minecraft.client.renderer.SubmitNodeCollector;

@Environment(EnvType.CLIENT)
public class ParticlesRenderState implements FabricRenderState {
	public final List<ParticleGroupRenderState> particles = new ArrayList<>();

	public void reset() {
		this.particles.forEach(ParticleGroupRenderState::clear);
		this.particles.clear();
	}

	public void add(final ParticleGroupRenderState state) {
		this.particles.add(state);
	}

	public void submit(final SubmitNodeCollector submitNodeCollector, final CameraRenderState camera) {
		for (ParticleGroupRenderState particle : this.particles) {
			particle.submit(submitNodeCollector, camera);
		}
	}
}
