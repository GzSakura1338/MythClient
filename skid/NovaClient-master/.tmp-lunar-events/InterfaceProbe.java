public class InterfaceProbe {
    interface Reader { int get(); }
    static int calls = 0;
    static int hook(Reader r) { return 7; }
    static void mark() { calls++; }
    static int invoke(Reader r, int n) {
        try { if (n > 0) return r.get() + n; return r.get() - n; }
        catch (IllegalStateException e) { return -1; }
    }
    public static void main(String[] args) {
        Reader r = () -> 2;
        if (args[0].equals("redirect")) {
            if (invoke(r, 3) != 10 || invoke(r, -2) != 9 || calls != 0) throw new AssertionError();
        } else {
            if (invoke(r, 3) != 5 || invoke(r, -2) != 4 || calls != 1) throw new AssertionError();
        }
        System.out.println("invokeinterface " + args[0] + " verified and executed");
    }
}
