//// ---- Tuning ---------------------------------------------------------
//// Starting guesses for the peak altitudes. They are adjusted
//// automatically from real readings after the first pass.
//float altMaxPos   = 60.0f;   // highest altitude above horizon
//float altMaxNeg   = 60.0f;   // deepest altitude below horizon (positive number)
//float deadband    = 0.15f;   // degrees of change needed to confirm direction
//float smoothing   = 0.20f;   // 0..1, low-pass factor (1 = no smoothing)
//bool  useArcSin   = true;    // true: arrow speed follows real sky motion
//
//// Local
//bool  started = false;
//bool  rising = true;
//bool  wasNegative = false;
//float filt = 0, refAlt = 0;
//float curPeakPos = 0, curPeakNeg = 0;
//float lastAngle = 90;
//
//// false: arrow angle linear with altitude
//
//// ---- Main call ------------------------------------------------------
//// alt      : MS.moon_altitude (degrees)
//// azimuth  : optional moon azimuth in degrees (0=N, 90=E). Pass NAN if
////            you don't have it.
//// returns  : arrow angle in degrees, 0..360 (0 = up, clockwise)
//float update(float alt, float azimuth = NAN) {
//  if (isnan(alt)) return lastAngle;
//
//  // 1. Smooth the raw value
//  if (!started) {
//    filt = alt;
//    refAlt = alt;
//    started = true;
//    // Unknown direction at power-up: guess from sign
//    // (better than nothing; corrected after 'deadband' degrees of change)
//    rising = (alt < 0);
//  } else {
//    filt += smoothing * (alt - filt);
//  }
//
//  // 2. Decide rising / falling
//  if (!isnan(azimuth)) {
//    float az = fmodf(azimuth + 360.0f, 360.0f);
//    rising = (az < 180.0f);          // east half of the sky = rising
//    refAlt = filt;
//  } else {
//    float d = filt - refAlt;
//    if (d >  deadband) {
//      rising = true;
//      refAlt = filt;
//    }
//    if (d < -deadband) {
//      rising = false;
//      refAlt = filt;
//    }
//  }
//
//  // 3. Learn the real peaks.
//  //    Track the extreme of the current half-pass; when the altitude
//  //    crosses zero, store it as the scale for the next pass of that sign.
//  if (filt > 0) {
//    if (filt > curPeakPos) curPeakPos = filt;
//    if (filt > altMaxPos)  altMaxPos  = filt;  // never let |n| exceed 1
//    if (wasNegative && curPeakNeg > 1.0f) {    // just rose above horizon
//      altMaxNeg  = curPeakNeg;
//      curPeakNeg = 0;
//    }
//    wasNegative = false;
//  } else if (filt < 0) {
//    if (-filt > curPeakNeg) curPeakNeg = -filt;
//    if (-filt > altMaxNeg)  altMaxNeg  = -filt;
//    if (!wasNegative && curPeakPos > 1.0f) {   // just set below horizon
//      altMaxPos  = curPeakPos;
//      curPeakPos = 0;
//    }
//    wasNegative = true;
//  }
//
//  // 4. Normalise to -1 .. +1
//  float n = (filt >= 0) ? filt / altMaxPos : filt / altMaxNeg;
//  n = constrain(n, -1.0f, 1.0f);
//
//  // 5. Phase angle f: -90 (lowest) .. 0 (horizon) .. +90 (highest)
//  float f = useArcSin ? degrees(asinf(n)) : n * 90.0f;
//
//  // 6. Map to arrow angle
//  //    rising : 180 -> 90 -> 0     (arrow = 90 - f)
//  //    falling:   0 -> 270 -> 180  (arrow = 270 + f)
//  float a = rising ? (90.0f - f) : (270.0f + f);
//  a = fmodf(a + 360.0f, 360.0f);
//
//  lastAngle = a;
//  return a;
//}
//
//bool isRising() {
//  return rising;
//}
//
//float smoothedAlt() const {
//  return filt;
//}
//
//// Arrow tip position for a display whose Y axis points down.
//static void tip(float angleDeg, int cx, int cy, int r, int &x, int &y) {
//  float rad = radians(angleDeg);
//  x = cx + (int)lroundf(r * sinf(rad));
//  y = cy - (int)lroundf(r * cosf(rad));
//}
//
