/// Fragment shader include; requires OgreUnifiedShader.h to be included first.

// 4x4 ordered (Bayer) dither matrix.
// bayer2 returns a value in [0, 1), bayer4 combines four of them to cover a 4x4 pixel block.
float pgBayer2(vec2 a) { a = floor(a); return fract(dot(a, vec2(0.5, a.y * 0.75))); }
float pgBayer4(vec2 a) { return pgBayer2(0.5 * a) * 0.25 + pgBayer2(a); }

/// Ordered dither threshold in [0, 15/16] for the given screen position.
/// invert should be 1.0 for geometry fading in with distance, so that its
/// dither pattern complements the one fading out; pass 0.0 otherwise.
float pgDitherThreshold(vec2 screenPos, float invert)
{
    return abs(invert * (15.0 / 16.0) - pgBayer4(screenPos));
}
