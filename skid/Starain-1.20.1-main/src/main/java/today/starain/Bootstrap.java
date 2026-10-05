package today.starain;

// 这是个跳板类 因为mixin不能混indy 这个类可以
// mixin初始化调用这个类 这个类去调用主类初始化
public class Bootstrap {
    public Bootstrap(String string) {
        launch();
    }

    private void launch() {
        Starain.newInstance();
    }
}
