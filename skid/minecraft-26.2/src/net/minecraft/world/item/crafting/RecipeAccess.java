package net.minecraft.world.item.crafting;

import net.fabricmc.fabric.api.recipe.v1.FabricRecipeAccess;
import net.minecraft.resources.ResourceKey;

public interface RecipeAccess extends FabricRecipeAccess {
	RecipePropertySet propertySet(ResourceKey<RecipePropertySet> id);

	SelectableRecipe.SingleInputSet<StonecutterRecipe> stonecutterRecipes();
}
