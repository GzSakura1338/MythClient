package today.starain.module.impl.misc;

import net.minecraft.client.multiplayer.PlayerInfo;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.DyeableLeatherItem;
import net.minecraft.world.item.ItemStack;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.value.impl.ModeValue;

import java.util.Objects;

/**
 * @Author：jiuxian_baka
 * @Date：2026/3/1 15:04
 */
public class Teams extends ClientModule {

    private final ModeValue mode = new ModeValue("Mode", "Scoreboard", new String[]{"Scoreboard", "Color", "Armor"}, this);

    public Teams() {
        setName("Teams");
        setCategory(Category.MISC);
    }


    public static boolean isSameTeam(Entity entity) {
        Teams teams = Starain.getInstance().getModuleManager().getModule(Teams.class);
        if (!teams.isEnabled()) {
            return false;
        } else if (entity instanceof Player player) {
            switch (teams.mode.get()) {
                case "Color" -> {
                    Integer c1 = player.getTeamColor();
                    Integer c2 = mc.player.getTeamColor();
                    return c1.equals(c2);
                }
                case "Scoreboard" -> {
                    String playerTeam = getTeam(player);
                    String targetTeam = getTeam(mc.player);
                    return Objects.equals(playerTeam, targetTeam);
                }
                case "Armor" -> {
                    Integer head = leatherColor(player.getItemBySlot(EquipmentSlot.HEAD));
                    Integer chest = leatherColor(player.getItemBySlot(EquipmentSlot.CHEST));
                    Integer selfHead = leatherColor(mc.player.getItemBySlot(EquipmentSlot.HEAD));
                    Integer selfChest = leatherColor(mc.player.getItemBySlot(EquipmentSlot.CHEST));

                    // 四件里但凡有一件不是染色皮革，就没有颜色可比，直接当不同队。
                    if (head == null || chest == null || selfHead == null || selfChest == null) {
                        return false;
                    }

                    return head.equals(selfHead) && chest.equals(selfChest);
                }
            }
        }
        return false;
    }

    /**
     * 染色皮革护甲的颜色；不是皮革、或者是皮革但没染过色，都返回 null。
     *
     * <p>只认 {@link DyeableLeatherItem}，所以铁/钻石/锁链这些走不到 {@code hasCustomColor}——
     * 它们压根没有这个接口。原版皮革不染色时 {@code getColor} 会返回默认的 {@code DEFAULT_LEATHER_COLOR}，
     * 那个值人人相同，拿来比队伍会把所有穿白皮的人都算成队友，所以这里用 {@code hasCustomColor} 先挡掉。
     */
    private static Integer leatherColor(ItemStack stack) {
        if (stack.getItem() instanceof DyeableLeatherItem dyeable && dyeable.hasCustomColor(stack)) {
            return dyeable.getColor(stack);
        }
        return null;
    }

    public static String getTeam(Entity entity) {
        PlayerInfo playerInfo = mc.getConnection().getPlayerInfo(entity.getUUID());
        if (playerInfo == null) {
            return null;
        } else {
            return playerInfo.getTeam() != null ? playerInfo.getTeam().getName() : null;
        }
    }

    @Override
    public String getSuffix() {
        return mode.get();
    }
}
