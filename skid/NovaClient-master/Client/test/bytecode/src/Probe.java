// Target for the bytecode rewriter's offline test. Every method here is a shape
// one of the real event hookups has to survive: a bare call, two calls with a
// limit on one of them, branches that need a StackMapTable, both switch forms
// (whose padding is relative to the start of the code array), an exception
// table, a backward branch, and locals that live across the insertion point.
public class Probe {

    private int yawValue = 7;
    private int pitchValue = 11;

    public int yaw() { return yawValue; }

    public int pitch() { return pitchValue; }

    // Never redirected: the test points a mismatched hook at it to check the
    // rewriter refuses, and reads it back to check the refusal left it alone.
    public int usePitch() { return pitch(); }

    // One call, redirected whole.
    public int useYaw() { return yaw() + 1; }

    // Two different callees; only the first yaw() is redirected, so a limit that
    // is honoured shows up as pitch() still being the real one.
    public int useBoth() { return yaw() * 100 + pitch(); }

    // Three returns, so StackMapTable frames exist at the branch targets.
    public int branchy(int k) {
        if (k > 0) return yaw();
        if (k < 0) return -yaw();
        return 0;
    }

    public int tableswitch(int k) {
        switch (k) {
            case 1: return yaw() + 10;
            case 2: return yaw() + 20;
            case 3: return yaw() + 30;
            default: return yaw();
        }
    }

    public int lookupswitch(int k) {
        switch (k) {
            case 100: return yaw() - 1;
            case 555: return yaw() - 2;
            default: return yaw();
        }
    }

    public int guarded(int k) {
        try {
            if (k == 0) throw new IllegalStateException("boom");
            return yaw() + k;
        } catch (IllegalStateException e) {
            return -1;
        }
    }

    public int loop(int n) {
        int s = 0;
        for (int i = 0; i < n; i++) s += yaw();
        return s;
    }

    // Locals that span the insertion point: the LocalVariableTable has to grow
    // with the code or they look like they die inside the hook.
    public String locals() {
        int a = yaw();
        int b = pitch();
        return a + ":" + b;
    }

    // One way out, so a TAIL insertion covers every path through it. This is the
    // shape StuckInBlockEvent's target has.
    public int tail(int k) {
        int s = k;
        s += yaw();
        return s;
    }

    public void tailVoid() {
        yaw();
    }

    // -- the hook-call insertion --------------------------------------------
    //
    // The shape Entity.m_7601_ has, one method per return type the emitter has
    // to pick a store opcode for: compute, call, store what the call handed
    // back into a field. Three of them because the interesting part is which
    // *category* the result is -- a reference, a float, an int -- and getting
    // that wrong is an `istore` where an `astore` belonged, which does not
    // misbehave at run time. It fails to verify, and the class never loads.
    //
    // So these are not here to check the arithmetic. They are here so the JVM's
    // own verifier reads the bytes the emitter produced, and it is the verifier
    // that has the final say on whether the emitter got the opcode right.

    public Object carriedObject;
    public float carriedFloat;
    public int carriedInt;

    public void step() { }

    public void carryObject(Object a, Object b) { step(); this.carriedObject = b; }

    public void carryFloat(Object a, float b) { step(); this.carriedFloat = b; }

    public void carryInt(Object a, int b) { step(); this.carriedInt = b; }

    // -- the cancellable head guard -----------------------------------------
    //
    // The shape LocalPlayer.m_8119_ has, and the one thing a head insertion
    // does that no other edit here does: the hook sits at offset 0, where the
    // only locals that exist are the receiver and the arguments, and it jumps
    // from there to the method's *last* return -- an instruction whose stack
    // map frame was computed for a program point partway through the method.
    //
    // Whether that branch is legal is the verifier's call, and it is not the
    // call one would guess. A frame that declares `top` for a slot the guard
    // has nothing in is fine -- `top` on the target side accepts anything. It
    // is the other direction that fails: a slot the *frame* declares a type
    // for, which the guard's own frame has as `top`, is
    //   Type top (current frame, locals[n]) is not assignable to <that type>
    // and the class does not load. So a guard is only legal where the frame on
    // the final return names nothing beyond the receiver and the arguments.
    //
    // The loop below is what puts a frame on the return at all: its exit
    // condition branches forward past the body, and with the loop as the last
    // statement that branch lands on the return. The two methods that take a
    // guard keep every local that outlives the loop in an argument, so the
    // frame at the return names exactly the parameters. The third declares its
    // array inside the method, so the frame at its return names a local the
    // guard cannot have -- and that one is here to be refused.
    //
    // `frozen` is read by the bridge hook rather than the constant 1, so one
    // patched class can be run both ways: with it false the body must run as
    // it always did, and with it true the method must do nothing at all.
    public static boolean frozen = false;
    public int touched = 0;

    public void guardedTick(int[] box) {
        for (int i = 0; i < 3; i++) {
            this.touched += box[0] + i;
        }
    }

    // The same, with more of them: the guard loads each local it hands the
    // hook, and the opcode for a reference at slot 2 is not the one for an int
    // at slot 1 or a reference at slot 0. Getting that wrong is not a wrong
    // answer, it is a class the verifier refuses.
    public void guardedTickArg(int k, Object box) {
        for (int i = 0; i < k; i++) {
            this.touched += ((String) box).length() + i;
        }
    }

    // Never guarded: its return frame declares a local that lives only inside
    // the method, which is the one shape a head guard cannot branch over.
    public void guardedTickBusy() {
        int[] box = new int[] { 5 };
        for (int i = 0; i < 3; i++) {
            this.touched += box[0] + i;
        }
    }
}
