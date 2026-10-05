package today.starain.ui.clickgui.opal.component;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public abstract class OpalComponent {

    protected float x, y, width, height;

    public static boolean isHovering(float x, float y, float width, float height, double mouseX, double mouseY) {
        return mouseX >= x && mouseY >= y && mouseX < x + width && mouseY < y + height;
    }

    public void setDimensions(float x, float y, float width, float height) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }

    public boolean isHovering(double mouseX, double mouseY) {
        return isHovering(x, y, width, height, mouseX, mouseY);
    }

    public void init() {
    }

    public void close() {
    }

    public abstract void render(int mouseX, int mouseY, float delta);

    public void mouseClicked(double mouseX, double mouseY, int button) {
    }

    public void mouseReleased(double mouseX, double mouseY, int button) {
    }

    public void mouseScrolled(double mouseX, double mouseY, double verticalAmount) {
    }

    public void keyPressed(int key) {
    }

    public void charTyped(int codepoint) {
    }

    public void setSearchQuery(String query) {
    }
}
