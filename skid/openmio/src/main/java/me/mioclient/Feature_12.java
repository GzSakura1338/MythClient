package me.mioclient;

import com.mojang.brigadier.arguments.IntegerArgumentType;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import me.mioclient.event.Listen;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_12.class */
public class Feature_12 extends Feature {
    public final List<String> list;
    public boolean flag;
    public int num;

    public Feature_12() {
        super("wait");
        this.list = new ArrayList();
        baritoneHelper.do1796(this);
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("ticks", IntegerArgumentType.integer(0)).executes(commandContext -> {
            this.num = ((Integer) commandContext.getArgument("ticks", Integer.class)).intValue();
            this.flag = true;
            return 1;
        }));
    }

    @Listen
    public void onEvent2(StringEvent stringEvent) {
        if (this.flag) {
            this.list.add(stringEvent.getString1957());
            stringEvent.do1162();
        }
    }

    @Listen
    public void onEvent(FontsEvent fontsEvent) {
        if (this.flag) {
            this.flag = false;
            ArrayList arrayList = new ArrayList(this.list);
            this.list.clear();
            TooltipsSearchHelper4_2 tooltipsSearchHelper4_2 = BaritoneHelper_3.tooltipsSearchHelper4_2;
            java.lang.Runnable runnable = () -> {
                Iterator it = arrayList.iterator();
                while (it.hasNext()) {
                    ChatFilterSearchHelper4_2.do2060((String) it.next());
                }
            };
            tooltipsSearchHelper4_2.do164(runnable, this.num);
        }
    }
}
