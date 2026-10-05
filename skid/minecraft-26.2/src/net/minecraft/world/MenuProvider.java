package net.minecraft.world;

import net.fabricmc.fabric.api.menu.v1.FabricMenuProvider;
import net.minecraft.network.chat.Component;
import net.minecraft.world.inventory.MenuConstructor;

public interface MenuProvider extends MenuConstructor, FabricMenuProvider {
	Component getDisplayName();
}
