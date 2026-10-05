package me.mioclient.module.render;

import com.mojang.blaze3d.platform.GlStateManager;
import com.mojang.blaze3d.systems.RenderSystem;
import it.unimi.dsi.fastutil.objects.Object2IntMap;
import it.unimi.dsi.fastutil.objects.ObjectArrayList;
import java.awt.Color;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.ArmorSearchHelper4;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.CrosshairHelper;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.FreecamHelper;
import me.mioclient.Helper_7;
import me.mioclient.MatrixStackEvent;
import me.mioclient.MixinMessageIndicatorHelper_2;
import me.mioclient.NameTagsHelper;
import me.mioclient.NameTagsHelper_2;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PingSpoofHelper;
import me.mioclient.SearchHelper4_8;
import me.mioclient.SearchHelper_2;
import me.mioclient.SearchHelper_3;
import me.mioclient.SearchHelper_4;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.RenderLabelEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.IllegalConstructorCall;
import me.mioclient.module.Module;
import me.mioclient.module.client.Fonts;
import me.mioclient.module.player.Freecam;
import net.minecraft.block.BlockState;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.network.PlayerListEntry;
import net.minecraft.client.render.DiffuseLighting;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.OverlayTexture;
import net.minecraft.client.render.model.json.ModelTransformationMode;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.enchantment.Enchantment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.registry.entry.RegistryEntry;
import net.minecraft.util.Formatting;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import org.joml.Vector3f;
import org.lwjgl.opengl.GL20;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/render/NameTags.class */
public class NameTags extends Module {
    public static final Identifier identifier = Identifier.of("mio", "textures/mio.png");
    public static Animations animations = (Animations) BaritoneHelper_3.baritoneHelper_4.getModule117(Animations.class);
    public static Freecam freecam = (Freecam) BaritoneHelper_3.baritoneHelper_4.getModule117(Freecam.class);
    public static Fonts fonts = (Fonts) BaritoneHelper_3.baritoneHelper_4.getModule117(Fonts.class);
    public static boolean flag = false;
    public Setting<Integer> range;
    public Setting<Float> scale;
    public Setting<Boolean> dead;
    public Setting<Boolean> info;
    public Setting<Boolean> name;
    public Setting<Boolean> ping;
    public Setting<Boolean> health;
    public Setting<Boolean> healthColor;
    public Setting<Boolean> gamemode;
    public Setting<Boolean> itemName;
    public Setting<Boolean> armor;
    public Setting<NameTagsMode> durability;
    public Setting<Boolean> items;
    public Setting<Boolean> showEating;
    public Setting<Boolean> enchants;
    public Setting<Boolean> hideMax;
    public Setting<Boolean> totemPops;
    public Setting<Boolean> mioCheck;
    public Setting<Boolean> colors;
    public Setting<Boolean> friend;
    public Setting<Boolean> enemy;
    public Setting<Color> fill;
    public Setting<Color> outline;
    public Setting<Boolean> smart;
    public Setting<Color> hole;
    public Setting<Color> phase;
    public Setting<Color> text;
    public Setting<Color> invisibles;
    public Setting<Color> sneak;
    public Setting<Color> eating;
    public final Color color;
    public final List<PlayerEntity> list;
    public boolean flag2;
    public boolean flag3;
    public boolean flag4;
    public Vector3f[] vector3fArr;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/render/NameTags$NameTagsMode.class */
    public enum NameTagsMode implements EnumSettingHelper {
        SHOW("Show"),
        ONLY("Only"),
        HIDE("Hide");

        public final String name;

        NameTagsMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/render/NameTags$NameTagsMode_2.class */
    public static enum NameTagsMode_2 {
        nameTagsMode_2 {
            @Override
            public float get1952(float f) {
                return (-f) * 0.5f;
            }
        },
        nameTagsMode_22 {
            @Override
            public float get1952(float f) {
                return 0.0f;
            }
        },
        nameTagsMode_23 {
            @Override
            public float get1952(float f) {
                return -f;
            }
        };

        public float get1952(float f) {
            return 0.0f;
        }

        public float get2063(String str, boolean z) {
            return get1952(z ? SearchHelper_4.minecraftClient.textRenderer.getWidth(str) : FontsSearchHelper4.fontsSearchHelper4.get1316(str));
        }
    }

    public NameTags() {
        super("NameTags", "Draws advanced nametags for players.", Category.RENDER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.color = new Color(153, 69, 58);
        this.list = new ObjectArrayList();
        this.vector3fArr = null;
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        flag = false;
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        synchronized (this.list) {
            this.list.clear();
            this.list.addAll(minecraftClient.world.getPlayers());
            this.list.sort(Comparator.comparing(playerEntity -> {
                return Double.valueOf(minecraftClient.gameRenderer.getCamera().getPos().squaredDistanceTo(playerEntity.getPos()));
            }));
        }
    }

    /* JADX WARN: Removed duplicated region for block: B:26:0x013f A[Catch: all -> 0x0327, TryCatch #0 {, blocks: (B:4:0x003e, B:5:0x0050, B:7:0x0061, B:9:0x007f, B:12:0x0092, B:14:0x00a1, B:17:0x00c2, B:19:0x00d3, B:21:0x0100, B:24:0x0123, B:26:0x013f, B:32:0x01a5, B:50:0x0323), top: B:3:0x003e }] */
    /* JADX WARN: Removed duplicated region for block: B:31:0x01a5 A[SYNTHETIC] */
    /* JADX WARN: Removed duplicated region for block: B:35:0x01a2 A[SYNTHETIC] */
    @Listen(get219= Helper_7.num4)
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        boolean z;
        boolean z2;
        this.vector3fArr = (Vector3f[]) Arrays.copyOf(RenderSystem.shaderLightDirections, RenderSystem.shaderLightDirections.length);
        Vec3d pos = minecraftClient.gameRenderer.getCamera().getPos();
        synchronized (this.list) {
            for (PlayerEntity playerEntity : this.list) {
                if (playerEntity != minecraftClient.player || freecam.isToggled()) {
                    if (!playerEntity.isDead() || this.dead.getValue().booleanValue()) {
                        if (BaritoneHelper_3.nameTagsSearchHelper4.is2305()) {
                            if (BaritoneHelper_3.nameTagsSearchHelper4.getIdentifier2309(playerEntity.getGameProfile().getName()) != null && this.mioCheck.getValue().booleanValue()) {
                                z = true;
                                this.flag2 = z;
                                if (SearchHelper4_8.is2492(playerEntity.getBoundingBox())) {
                                    if (minecraftClient.gameRenderer.getCamera().getPos().distanceTo(playerEntity.getPos()) <= this.range.getValue().intValue()) {
                                        z2 = false;
                                        if (!z2) {
                                            float f = SearchHelper_2.get536();
                                            double lerp = MathHelper.lerp(f, playerEntity.lastRenderX, playerEntity.getX()) - pos.x;
                                            double lerp2 = MathHelper.lerp(f, playerEntity.lastRenderY, playerEntity.getY()) - pos.y;
                                            double lerp3 = MathHelper.lerp(f, playerEntity.lastRenderZ, playerEntity.getZ()) - pos.z;
                                            Vec3d positionOffset = minecraftClient.getEntityRenderDispatcher().getRenderer((Entity) playerEntity).getPositionOffset((Entity) playerEntity, f);
                                            double x = lerp + positionOffset.getX();
                                            double y = lerp2 + positionOffset.getY();
                                            double z3 = lerp3 + positionOffset.getZ();
                                            inner_3.getMatrixStack472().push();
                                            inner_3.getMatrixStack472().translate(x, y, z3);
                                            do1308(inner_3.getDrawContext474(), inner_3.getMatrixStack472(), playerEntity);
                                            inner_3.getMatrixStack472().pop();
                                        }
                                    }
                                }
                                z2 = true;
                                if (!z2) {
                                }
                            }
                        }
                        z = false;
                        this.flag2 = z;
                        if (SearchHelper4_8.is2492(playerEntity.getBoundingBox())) {
                        }
                        z2 = true;
                        if (!z2) {
                        }
                    }
                }
            }
        }
        RenderSystem.enablePolygonOffset();
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(-910746880));
        GL20.glDepthRange(0.0d, Double.longBitsToDouble(4591870180066957722L));
        RenderSystem.enableBlend();
        RenderSystem.blendFunc(GlStateManager.SrcFactor.SRC_ALPHA, GlStateManager.DstFactor.ONE_MINUS_SRC_ALPHA);
        DiffuseLighting.disableGuiDepthLighting();
        inner_3.getDrawContext474().getVertexConsumers().draw();
        RenderSystem.setShaderLights(this.vector3fArr[0], this.vector3fArr[1]);
        RenderSystem.disableBlend();
        CrosshairHelper.do1713(false);
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(-979615744));
        FontsSearchHelper4.fontsSearchHelper4.do1597();
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1167867904));
        GL20.glDepthRange(0.0d, Double.longBitsToDouble(4607182418800017408L));
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1236736768));
        RenderSystem.disablePolygonOffset();
    }

    @Listen
    public void onRenderLabel(RenderLabelEvent renderLabelEvent) {
        if (renderLabelEvent.getEntity181() instanceof PlayerEntity) {
            renderLabelEvent.do1162();
        }
    }

    public void do1307(MatrixStack matrixStack, Vec3d vec3d) {
        RenderSystem.enablePolygonOffset();
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(-913146880));
        GL20.glDepthRange(0.0d, Double.longBitsToDouble(4591870180066957722L));
        Iterator<PlayerEntity> it = this.list.iterator();
        while (it.hasNext()) {
            ClientPlayerEntity clientPlayerEntity = (ClientPlayerEntity)((PlayerEntity) it.next());
            if (minecraftClient.player != clientPlayerEntity) {
                float f = (float) PingSpoofHelper.get377(minecraftClient.gameRenderer.getCamera().getPos(), ((PlayerEntity) clientPlayerEntity).getPos(), this.scale.getValue().floatValue());
                float f2 = SearchHelper_2.get536();
                double lerp = MathHelper.lerp(f2, ((PlayerEntity) clientPlayerEntity).lastRenderX, ((PlayerEntity) clientPlayerEntity).getX()) - vec3d.x;
                double lerp2 = MathHelper.lerp(f2, ((PlayerEntity) clientPlayerEntity).lastRenderY, ((PlayerEntity) clientPlayerEntity).getY()) - vec3d.y;
                double lerp3 = MathHelper.lerp(f2, ((PlayerEntity) clientPlayerEntity).lastRenderZ, ((PlayerEntity) clientPlayerEntity).getZ()) - vec3d.z;
                Vec3d positionOffset = minecraftClient.getEntityRenderDispatcher().getRenderer((Entity) clientPlayerEntity).getPositionOffset((Entity) clientPlayerEntity, f2);
                double x = lerp + positionOffset.getX();
                double y = lerp2 + positionOffset.getY();
                double z = lerp3 + positionOffset.getZ();
                matrixStack.push();
                matrixStack.translate(x, y, z);
                matrixStack.push();
                matrixStack.translate(0.0d, (((PlayerEntity) clientPlayerEntity).getHeight() * (animations.is999() && clientPlayerEntity != minecraftClient.player ? animations.playerScale.getValue().floatValue() : Float.intBitsToFloat(1065353216))) + Float.intBitsToFloat(1056964608), 0.0d);
                matrixStack.multiply(minecraftClient.getEntityRenderDispatcher().getRotation());
                matrixStack.scale((-f) * Float.intBitsToFloat(1020054733), (-f) * Float.intBitsToFloat(1020054733), f * Float.intBitsToFloat(1020054733));
                float longBitsToDouble = (float) ((-get1316(getString1312((PlayerEntity) clientPlayerEntity))) * Double.longBitsToDouble(4602678819172646912L));
                SearchHelper_2.searchHelper_2.do546(matrixStack, longBitsToDouble - Float.intBitsToFloat(1065353216), Float.intBitsToFloat(-1082130432), (longBitsToDouble * Float.intBitsToFloat(-1082130432)) + Float.intBitsToFloat(1065353216), get990() + Float.intBitsToFloat(1065353216), Color.white);
                matrixStack.pop();
                matrixStack.pop();
            }
        }
        GL20.glDepthRange(0.0d, Double.longBitsToDouble(4607182418800017408L));
        RenderSystem.disablePolygonOffset();
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1234336768));
    }

    public void do1308(DrawContext drawContext, MatrixStack matrixStack, PlayerEntity playerEntity) {
        this.flag3 = false;
        this.flag4 = false;
        float height = (playerEntity.getHeight() * (animations.is999() && playerEntity != minecraftClient.player ? animations.playerScale.getValue().floatValue() : Float.intBitsToFloat(1065353216))) + Float.intBitsToFloat(1056964608);
        float f = (float) PingSpoofHelper.get377(minecraftClient.gameRenderer.getCamera().getPos(), playerEntity.getPos(), this.scale.getValue().floatValue());
        matrixStack.push();
        matrixStack.translate(0.0f, height, 0.0f);
        matrixStack.multiply(minecraftClient.getEntityRenderDispatcher().getRotation());
        matrixStack.scale(f * Float.intBitsToFloat(1020054733), (-f) * Float.intBitsToFloat(1020054733), (-f) * Float.intBitsToFloat(1020054733));
        String string1312 = getString1312(playerEntity);
        float longBitsToDouble = (float) ((-get1316(string1312)) * Double.longBitsToDouble(4602678819172646912L));
        RenderSystem.enablePolygonOffset();
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(-910746880));
        GL20.glDepthRange(0.0d, Double.longBitsToDouble(4591870180066957722L));
        Color value = this.outline.getValue();
        if (this.smart.getValue().booleanValue()) {
            if (is1317((Entity) playerEntity)) {
                value = this.phase.getValue();
            } else {
                if (BaritoneHelper_3.holeSnapSearchHelper4_5.is2723(playerEntity.getBlockPos())) {
                    value = this.hole.getValue();
                }
            }
        }
        float intBitsToFloat = Float.intBitsToFloat(1056964608);
        CrosshairHelper.do1707(matrixStack, longBitsToDouble - Float.intBitsToFloat(1065353216), Float.intBitsToFloat(-1082130432), (longBitsToDouble * Float.intBitsToFloat(-1082130432)) + Float.intBitsToFloat(1065353216), get990() + Float.intBitsToFloat(1065353216), this.fill.getValue());
        CrosshairHelper.do1706(matrixStack, (longBitsToDouble - Float.intBitsToFloat(1073741824)) + intBitsToFloat, Float.intBitsToFloat(-1073741824) + intBitsToFloat, (longBitsToDouble * Float.intBitsToFloat(-1082130432)) + Float.intBitsToFloat(1065353216), get990() + Float.intBitsToFloat(1065353216), intBitsToFloat, value);
        Color value2 = this.text.getValue();
        if (playerEntity.isSneaking()) {
            value2 = this.sneak.getValue();
        }
        if (playerEntity.isInvisible()) {
            value2 = this.invisibles.getValue();
        }
        if (BaritoneHelper_3.searchHelper4_14.is520(playerEntity) && this.friend.getValue().booleanValue()) {
            value2 = BaritoneHelper_3.searchHelper4_14.getColor528();
        }
        if (BaritoneHelper_3.searchHelper4_14.is522(playerEntity) && this.enemy.getValue().booleanValue()) {
            value2 = BaritoneHelper_3.searchHelper4_14.getColor529();
        }
        boolean isToggled = fonts.isToggled();
        FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, string1312, longBitsToDouble, Float.intBitsToFloat(1065353216), value2);
        flag = true;
        do1309(drawContext, matrixStack, playerEntity);
        flag = false;
        if (this.flag2) {
            matrixStack.push();
            float intBitsToFloat2 = Float.intBitsToFloat(1032847360);
            matrixStack.translate(longBitsToDouble - Float.intBitsToFloat(1082130432), Float.intBitsToFloat(-1065353216), 0.0f);
            matrixStack.scale(intBitsToFloat2, intBitsToFloat2, intBitsToFloat2);
            if (isToggled) {
                matrixStack.translate(Double.longBitsToDouble(-4616189618054758400L), Double.longBitsToDouble(4608533498688228557L) + fonts.shift.getValue().intValue(), 0.0d);
            }
            RenderSystem.setShader(GameRenderer::getPositionTexColorProgram);
            GlStateManager._blendFunc(770, 771);
            GlStateManager._texParameter(3553, 10240, 9729);
            drawContext.drawTexture(identifier, 0, 0, 0, 0, 256, 256);
            GlStateManager._texParameter(3553, 10240, 9728);
            matrixStack.pop();
        }
        GL20.glDepthRange(0.0d, Double.longBitsToDouble(4607182418800017408L));
        RenderSystem.disablePolygonOffset();
        RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1236736768));
        matrixStack.pop();
    }

    public void do1309(DrawContext drawContext, MatrixStack matrixStack, PlayerEntity playerEntity) {
        ArrayList<ItemStack> arrayList = new ArrayList();
        if (this.items.getValue().booleanValue() && !playerEntity.getOffHandStack().isEmpty()) {
            arrayList.add(playerEntity.getOffHandStack());
        }
        for (ItemStack itemStack : playerEntity.getArmorItems()) {
            if (!itemStack.isEmpty() && this.armor.getValue().booleanValue()) {
                arrayList.add(itemStack);
            }
        }
        if (this.items.getValue().booleanValue() && !playerEntity.getMainHandStack().isEmpty()) {
            arrayList.add(playerEntity.getMainHandStack());
        }
        int i = -3;
        for (ItemStack itemStack2 : arrayList) {
            int size = IllegalConstructorCall.getMap1419(itemStack2).size();
            if (this.hideMax.getValue().booleanValue() && NameTagsHelper_2.is2028(itemStack2)) {
                size = 0;
            }
            if (size > i) {
                i = size;
            }
        }
        int max = Math.max(i, 0);
        if (this.durability.getValue() == NameTagsMode.ONLY || !this.enchants.getValue().booleanValue()) {
            max = 0;
        }
        float f = -Math.max((((max * get990()) * Float.intBitsToFloat(1056964608)) / Float.intBitsToFloat(1067030938)) / Float.intBitsToFloat(1099563008), Float.intBitsToFloat(1058642330));
        for (int i2 = 0; i2 < arrayList.size(); i2++) {
            do1310(drawContext, playerEntity, matrixStack, (ItemStack) arrayList.get((arrayList.size() - 1) - i2), (i2 + Float.intBitsToFloat(1056964608)) - (arrayList.size() * Float.intBitsToFloat(1056964608)), f, Float.intBitsToFloat(1099563008));
        }
        if (playerEntity.getMainHandStack().isEmpty() || !this.itemName.getValue().booleanValue()) {
            return;
        }
        float intBitsToFloat = ((f * Float.intBitsToFloat(1099563008)) - Float.intBitsToFloat(1082130432)) - get990();
        if (!this.flag3) {
            intBitsToFloat = -get990();
        }
        if (this.flag4) {
            intBitsToFloat -= get990() * Float.intBitsToFloat(1056964608);
            if (!this.flag3) {
                intBitsToFloat -= Float.intBitsToFloat(1065353216);
            }
        }
        do1311(drawContext, Formatting.strip(playerEntity.getMainHandStack().getName().getString()), 0.0f, intBitsToFloat, Double.longBitsToDouble(4627005293803339776L), Color.white, false, NameTagsMode_2.nameTagsMode_2);
    }

    public void do1310(DrawContext drawContext, PlayerEntity playerEntity, MatrixStack matrixStack, ItemStack itemStack, float f, float f2, float f3) {
        float intBitsToFloat = Float.intBitsToFloat(1058642330);
        int i = ArmorSearchHelper4.get1905(itemStack);
        if (this.items.getValue().booleanValue() && this.showEating.getValue().booleanValue() && playerEntity.isUsingItem() && playerEntity.getStackInHand(playerEntity.getActiveHand()) == itemStack && itemStack.contains(DataComponentTypes.FOOD)) {
            CrosshairHelper.do1708(matrixStack, (f - Float.intBitsToFloat(1055286886)) * f3, (f2 * f3) - Float.intBitsToFloat(1089575322), ((f - Float.intBitsToFloat(1055286886)) + (Float.intBitsToFloat(1063675494) * MathHelper.clamp(((float) (System.currentTimeMillis() - ((NameTagsHelper) playerEntity).mio$getLastEatingTime())) / ((itemStack.getMaxUseTime((LivingEntity) playerEntity) * Float.intBitsToFloat(1112014848)) + Float.intBitsToFloat(1112014848)), 0.0f, Float.intBitsToFloat(1065353216)))) * f3, (f2 * f3) + Float.intBitsToFloat(1089575322), MixinMessageIndicatorHelper_2.get819(this.eating.getValue(), Float.intBitsToFloat(1045220557)));
        }
        boolean z = false;
        if (this.durability.getValue() != NameTagsMode.ONLY || (!(itemStack.getItem() instanceof ArmorItem) && !itemStack.isOf(Items.ELYTRA))) {
            z = true;
            this.flag3 = true;
            RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1167867904));
            matrixStack.push();
            matrixStack.translate(f * f3, f2 * f3, 0.0f);
            matrixStack.scale(-f3, -f3, Float.intBitsToFloat(981668463));
            minecraftClient.getItemRenderer().renderItem(itemStack, ModelTransformationMode.FIXED, false, matrixStack, drawContext.getVertexConsumers(), 15728880, OverlayTexture.DEFAULT_UV, minecraftClient.getItemRenderer().getModel(itemStack, minecraftClient.world, (LivingEntity) null, 0));
            matrixStack.pop();
            RenderSystem.polygonOffset(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(-979615744));
        }
        if (itemStack.getCount() > 1) {
            do1311(drawContext, String.valueOf(itemStack.getCount()), (f + Float.intBitsToFloat(1056964608)) * f3, (f2 * f3) + Float.intBitsToFloat(1065353216), f3 * Double.longBitsToDouble(4612248968380809216L), Color.white, true, NameTagsMode_2.nameTagsMode_23);
        }
        if (itemStack.isItemBarVisible() && z) {
            float intBitsToFloat2 = ((int) (f + Float.intBitsToFloat(1045220557))) * f3;
            float intBitsToFloat3 = (f2 * f3) + Float.intBitsToFloat(1085276160);
            CrosshairHelper.do1707(matrixStack, (f - Float.intBitsToFloat(1055286886)) * f3, intBitsToFloat3 - Float.intBitsToFloat(1028443341), (f + Float.intBitsToFloat(1055286886)) * f3, intBitsToFloat3 + Float.intBitsToFloat(1073741824), new Color(-16777216));
            CrosshairHelper.do1707(matrixStack, (f - Float.intBitsToFloat(1055286886)) * f3, intBitsToFloat3 - Float.intBitsToFloat(1028443341), (f + MathHelper.lerp(itemStack.getItemBarStep() / Float.intBitsToFloat(1095761920), Float.intBitsToFloat(-1092196762), Float.intBitsToFloat(1055286886))) * f3, intBitsToFloat3 + Float.intBitsToFloat(1065772646), new Color(itemStack.getItemBarColor(), false));
        }
        Set<Object2IntMap.Entry> enchantmentEntries = (Set)(itemStack.getEnchantments().getEnchantmentEntries());
        float intBitsToFloat4 = (-intBitsToFloat) * Float.intBitsToFloat(1073741824);
        if (itemStack.isOf(Items.ENCHANTED_GOLDEN_APPLE) && z) {
            do1311(drawContext, "God", f * f3, ((f2 * f3) - Float.intBitsToFloat(1065353216)) + (get990() * intBitsToFloat * intBitsToFloat4), f3 * Float.intBitsToFloat(1067869798), this.color, false, NameTagsMode_2.nameTagsMode_23);
            return;
        }
        if (z && this.enchants.getValue().booleanValue()) {
            boolean z2 = this.hideMax.getValue().booleanValue() && NameTagsHelper_2.is2028(itemStack);
            if (z2) {
                do1311(drawContext, "Max", (f - Float.intBitsToFloat(1056964608)) * f3, ((f2 * f3) - Float.intBitsToFloat(1073741824)) + (((get990() * intBitsToFloat) / Float.intBitsToFloat(1067030938)) * intBitsToFloat4), f3 * Float.intBitsToFloat(1067869798), Color.RED, false, NameTagsMode_2.nameTagsMode_22);
            }
            for (Object2IntMap.Entry entry : enchantmentEntries) {
                if (z2) {
                    break;
                }
                boolean z3 = ((Enchantment) ((RegistryEntry) entry.getKey()).value()).getMaxLevel() == 1;
                String string = Enchantment.getName((RegistryEntry) entry.getKey(), entry.getIntValue()).getString();
                String substring = string.substring(0, Math.min(z3 ? 3 : 2, string.length()));
                if (!substring.equalsIgnoreCase("Cu")) {
                    if (!z3) {
                        substring = new ArgumentTypeHelper().getArgumentTypeHelper2906(entry.getIntValue()).getArgumentTypeHelper2919(substring).getString2921("\u0001\u0001");
                    }
                    do1311(drawContext, substring, (f - Float.intBitsToFloat(1056964608)) * f3, ((f2 * f3) - Float.intBitsToFloat(1073741824)) + (((get990() * intBitsToFloat) / Float.intBitsToFloat(1067030938)) * intBitsToFloat4), f3 * Float.intBitsToFloat(1067869798), Color.WHITE, false, NameTagsMode_2.nameTagsMode_22);
                    intBitsToFloat4 += Float.intBitsToFloat(1065353216);
                }
            }
        }
        if (!itemStack.isDamageable() || this.durability.getValue() == NameTagsMode.HIDE) {
            return;
        }
        float intBitsToFloat5 = ((f2 * f3) - Float.intBitsToFloat(1082130432)) - get990();
        if (!z) {
            intBitsToFloat5 = f2 - get990();
        }
        this.flag4 = true;
        do1311(drawContext, String.format("%d%s", Integer.valueOf(i), "%"), ((float) ((f - FreecamHelper.val2) * f3)) + Float.intBitsToFloat(1065353216), intBitsToFloat5, f3 * Float.intBitsToFloat(1069547520), new Color(itemStack.getItemBarColor(), false), false, NameTagsMode_2.nameTagsMode_22);
    }

    public void do1311(DrawContext drawContext, String str, float f, float f2, double d, Color color, boolean z, NameTagsMode_2 nameTagsMode_2) {
        MatrixStack matrices = drawContext.getMatrices();
        matrices.push();
        matrices.translate(f, f2, 0.0f);
        matrices.scale(Float.intBitsToFloat(1020054733) * ((float) d), Float.intBitsToFloat(1020054733) * ((float) d), Float.intBitsToFloat(1065353216));
        float f3 = nameTagsMode_2.get2063(str, z);
        if (z) {
            CrosshairHelper.do1711(drawContext.getMatrices(), str, (int) f3, 0, color.hashCode(), true);
        } else {
            FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, str, f3, 0.0f, color);
        }
        matrices.pop();
    }

    public String getString1312(PlayerEntity playerEntity) {
        int i;
        StringBuilder sb = new StringBuilder();
        if (this.flag2) {
            sb.append(Formatting.RESET);
            sb.append("   ");
        }
        if (this.name.getValue().booleanValue()) {
            sb.append(BaritoneHelper_3.notificationsHelper.getString398(playerEntity.getGameProfile().getName()));
        }
        if (this.gamemode.getValue().booleanValue()) {
            sb.append(" [");
            try {
                PlayerListEntry playerListEntry = minecraftClient.player.networkHandler.getPlayerListEntry(playerEntity.getGameProfile().getId());
                sb.append(playerListEntry == null ? "S" : getString1315(playerListEntry.getGameMode().getName()));
            } catch (Throwable th) {
                sb.append(0);
            }
            sb.append("]");
        }
        if (this.ping.getValue().booleanValue()) {
            sb.append(" ");
            try {
                PlayerListEntry playerListEntry2 = minecraftClient.player.networkHandler.getPlayerListEntry(playerEntity.getGameProfile().getId());
                sb.append(playerListEntry2 == null ? 0 : playerListEntry2.getLatency());
            } catch (Throwable th2) {
                sb.append(0);
            }
            sb.append("ms");
        }
        if (this.health.getValue().booleanValue()) {
            float f = SearchHelper_3.get644((Entity) playerEntity);
            if (this.healthColor.getValue().booleanValue()) {
                sb.append(getFormatting1313(f));
            }
            sb.append(" ").append(String.format("%.1f", Float.valueOf(f))).append(Formatting.RESET);
        }
        if (this.totemPops.getValue().booleanValue() && (i = BaritoneHelper_3.logoutSpotsHelper.get895(playerEntity)) > 0) {
            sb.append(getFormatting1314(i)).append(" -").append(i);
        }
        return sb.toString().trim();
    }

    public Formatting getFormatting1313(double d) {
        return d >= Double.longBitsToDouble(4626322717216342016L) ? Formatting.GREEN : d >= Double.longBitsToDouble(4625196817309499392L) ? Formatting.DARK_GREEN : d >= Double.longBitsToDouble(4621819117588971520L) ? Formatting.GOLD : d >= Double.longBitsToDouble(4616189618054758400L) ? Formatting.RED : Formatting.DARK_RED;
    }

    public static Formatting getFormatting1314(int i) {
        return i > 6 ? Formatting.DARK_RED : i > 3 ? Formatting.GOLD : Formatting.YELLOW;
    }

    public String getString1315(String str) {
        String lowerCase = str.toLowerCase(Locale.ROOT);
        int z = -1;
        switch (lowerCase.hashCode()) {
            case -1684593425:
                if (lowerCase.equals("spectator")) {
                    z = 3;
                    break;
                }
                break;
            case -1600582850:
                if (lowerCase.equals("survival")) {
                    z = 0;
                    break;
                }
                break;
            case -694094064:
                if (lowerCase.equals("adventure")) {
                    z = 2;
                    break;
                }
                break;
            case 1820422063:
                if (lowerCase.equals("creative")) {
                    z = 1;
                    break;
                }
                break;
        }
        switch (z) {
            case 0:
                return "S";
            case 1:
                return "C";
            case 2:
                return "A";
            case 3:
                return "SP";
            default:
                return "";
        }
    }

    public float get990() {
        return FontsSearchHelper4.fontsSearchHelper4.get93();
    }

    public float get1316(String str) {
        return FontsSearchHelper4.fontsSearchHelper4.get1316(str);
    }

    public boolean is1317(Entity entity) {
        float width = entity.getDimensions(entity.getPose()).width() * Float.intBitsToFloat(1061997773);
        return BlockPos.stream(Box.of(entity.getBoundingBox().getCenter(), width, entity.getHeight() * Float.intBitsToFloat(1056964608), width)).anyMatch(blockPos -> {
            BlockState blockState = minecraftClient.world.getBlockState(blockPos);
            return !blockState.isAir() && blockState.shouldSuffocate(minecraftClient.world, blockPos);
        });
    }
}
