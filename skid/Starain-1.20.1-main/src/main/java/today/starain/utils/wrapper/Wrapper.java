package today.starain.utils.wrapper;

import net.minecraft.client.Minecraft;
import today.starain.Starain;

import java.io.File;

public interface Wrapper {
    Minecraft mc = Minecraft.getInstance();

    static File resolveConfigFolder() {
        return new File(Minecraft.getInstance().gameDirectory, Starain.NAME).getAbsoluteFile();
    }
}
