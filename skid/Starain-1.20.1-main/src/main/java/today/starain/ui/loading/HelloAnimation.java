package today.starain.ui.loading;

import io.github.humbleui.skija.Canvas;
import io.github.humbleui.skija.Matrix33;
import io.github.humbleui.skija.Paint;
import io.github.humbleui.skija.PaintMode;
import io.github.humbleui.skija.PaintStrokeCap;
import io.github.humbleui.skija.PaintStrokeJoin;
import io.github.humbleui.skija.Path;
import io.github.humbleui.skija.PathMeasure;
import io.github.humbleui.types.Rect;
import today.starain.utils.render.skia.Skia;

public final class HelloAnimation {

    public static final long STROKE_MILLIS = 2000L;
    public static final long HOLD_MILLIS = 2450L;

    private static final String GLYPH_ENTRY =
            "M-109.06069946289062,95.92639923095703 C1.9544999599456787,157.6403045654297 103.11389923095703,236.9969940185" +
            "547 217.881103515625,372.07550048828125 C296,464.2846984863281 337.9999084472656,569.5725708007812 340,642.193" +
            "9697265625 C341,696.1920166015625 314.6702880859375,737.156005859375 266,737.156005859375 C212,737.15600585937" +
            "5 178,696.1920166015625 157,602.1610107421875 C134,498.82000732421875 117,380.239990234375 74,0";

    private static final String GLYPH_BODY =
            "M78.21453094482422,37.160953521728516 C100.22924041748047,230.68260192871094 184,372 291,372 C355,372 395.6745" +
            "910644531,321 384.1253967285156,248 C377.6238098144531,205 370.0873107910156,161 361.3063049316406,110 C351.07" +
            "14111328125,46 380.3254089355469,-4 468.96173095703125,-4 C598.2246704101562,-4 739.2435302734375,67.833816528" +
            "32031 811.4124145507812,179.0941619873047 C836,217 846,251 847,284 C848,344 814,389 754,389 C678,389 620,303 6" +
            "20,193 C620,75 684,-8 819.9180908203125,-8 C1004.7244873046875,-8 1209.4246826171875,213.84754943847656 1303.4" +
            "808349609375,461.42327880859375 C1330.037353515625,531.3258056640625 1340,596.2349243164062 1340,641.593994140" +
            "625 C1340,695.3764038085938 1323,736.673583984375 1275,736.673583984375 C1228,736.673583984375 1197,700.178405" +
            "7617188 1169,642.5543823242188 C1136.1939697265625,575.7216186523438 1111.927734375,479.32598876953125 1102,37" +
            "0.3599853515625 C1077,96.94000244140625 1133,-4 1266.152099609375,-4 C1427.6083984375,-4 1607.1151123046875,22" +
            "0.92921447753906 1698.771728515625,462.18878173828125 C1725.037353515625,531.3258056640625 1735,596.2349243164" +
            "062 1735,641.593994140625 C1735,695.3764038085938 1718,736.673583984375 1670,736.673583984375 C1623,736.673583" +
            "984375 1592,700.1784057617188 1564,642.5543823242188 C1531.1939697265625,575.7216186523438 1506.927734375,479." +
            "32598876953125 1497,370.3599853515625 C1472,96.94000244140625 1528,-4 1646.906005859375,-4 C1765.623779296875," +
            "-4 1830.114990234375,99.48485565185547 1868.77880859375,209.3712158203125 C1907,318 1954,385 2052,385 C2133,38" +
            "5 2197,325 2197,212 C2197,87 2115.90087890625,-7 2013.41845703125,-8 C1923.234130859375,-9 1864,64 1870,174 C1" +
            "877,296 1951,385 2048,385 C2104,385 2151.03564453125,360.1071472167969 2188,333 C2288.21435546875,259.89285278" +
            "32031 2365.4287109375,305.0714416503906 2395,377.3571472167969";

    private static final float FLIP_Y = 728.156006F;
    private static final float DESIGN_STROKE = 60.0F;
    private static final float MAX_GLYPH_WIDTH = 430.0F;
    private static final float GLYPH_WIDTH_RATIO = 0.48F;
    private static final float GLYPH_CENTER_RATIO = 0.46F;
    private static final float BAR_WIDTH_RATIO = 0.26F;
    private static final float MAX_BAR_WIDTH = 210.0F;
    private static final float BAR_HEIGHT = 3.0F;
    private static final float BAR_CENTER_RATIO = 0.79F;

    private static Path[] strokes;
    private static Path[] segments;
    private static PathMeasure[] measures;
    private static float[] lengths;
    private static float total;
    private static Rect bounds;
    private static boolean failed;

    private HelloAnimation() {
    }

    public static float strokeProgress(long elapsed) {
        if (elapsed <= 0L) return 0.0F;
        if (elapsed >= STROKE_MILLIS) return 1.0F;
        return (float) elapsed / (float) STROKE_MILLIS;
    }

    public static void draw(float width, float height, float progress, float alpha, float load) {
        int opacity = (int) (clamp(alpha) * 255.0F);
        if (opacity <= 0) return;

        build();
        if (!failed) {
            float scale = Math.min(width * GLYPH_WIDTH_RATIO, MAX_GLYPH_WIDTH) / bounds.getWidth();
            float glyphWidth = bounds.getWidth() * scale;
            float glyphHeight = bounds.getHeight() * scale;
            float originX = (width - glyphWidth) * 0.5F - bounds.getLeft() * scale;
            float originY = height * GLYPH_CENTER_RATIO - glyphHeight * 0.5F - bounds.getTop() * scale;

            Canvas canvas = Skia.getCanvas();
            canvas.save();
            canvas.translate(originX, originY);
            canvas.scale(scale, scale);
            reveal(canvas, total * clamp(progress), opacity);
            canvas.restore();
        }

        bar(width, height, load, opacity);
    }

    private static void reveal(Canvas canvas, float drawn, int opacity) {
        for (int i = 0; i < strokes.length; i++) {
            if (drawn <= 0.0F) return;
            if (drawn >= lengths[i]) {
                stroke(canvas, strokes[i], opacity);
            } else {
                segments[i].reset();
                if (measures[i].getSegment(0.0F, drawn, segments[i], true)) {
                    stroke(canvas, segments[i], opacity);
                }
            }
            drawn -= lengths[i];
        }
    }

    private static void stroke(Canvas canvas, Path path, int opacity) {
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setMode(PaintMode.STROKE);
        paint.setStrokeWidth(DESIGN_STROKE);
        paint.setStrokeCap(PaintStrokeCap.ROUND);
        paint.setStrokeJoin(PaintStrokeJoin.ROUND);
        paint.setColor((opacity << 24) | 0xFFFFFF);
        canvas.drawPath(path, paint);
        paint.close();
    }

    private static void bar(float width, float height, float load, int opacity) {
        float barWidth = Math.min(width * BAR_WIDTH_RATIO, MAX_BAR_WIDTH);
        float x = (width - barWidth) * 0.5F;
        float y = height * BAR_CENTER_RATIO;
        float radius = BAR_HEIGHT * 0.5F;
        Skia.drawRoundedRect(x, y, barWidth, BAR_HEIGHT, radius, ((opacity / 5) << 24) | 0xFFFFFF);
        float filled = Math.max(BAR_HEIGHT, barWidth * clamp(load));
        Skia.drawRoundedRect(x, y, filled, BAR_HEIGHT, radius, (opacity << 24) | 0xFFFFFF);
    }

    private static float clamp(float value) {
        if (value <= 0.0F) return 0.0F;
        return Math.min(value, 1.0F);
    }

    private static void build() {
        if (strokes != null || failed) return;

        Path first = Path.makeFromSVGString(GLYPH_ENTRY);
        Path second = Path.makeFromSVGString(GLYPH_BODY);
        if (first == null || second == null) {
            failed = true;
            return;
        }

        Matrix33 flip = new Matrix33(1.0F, 0.0F, 0.0F, 0.0F, -1.0F, FLIP_Y, 0.0F, 0.0F, 1.0F);
        first.transform(flip);
        second.transform(flip);

        strokes = new Path[]{first, second};
        segments = new Path[]{new Path(), new Path()};
        measures = new PathMeasure[strokes.length];
        lengths = new float[strokes.length];
        total = 0.0F;
        for (int i = 0; i < strokes.length; i++) {
            measures[i] = new PathMeasure(strokes[i], false, 1.0F);
            lengths[i] = measures[i].getLength();
            total += lengths[i];
        }

        Rect a = first.getBounds();
        Rect b = second.getBounds();
        bounds = Rect.makeLTRB(
                Math.min(a.getLeft(), b.getLeft()),
                Math.min(a.getTop(), b.getTop()),
                Math.max(a.getRight(), b.getRight()),
                Math.max(a.getBottom(), b.getBottom()));
    }
}
