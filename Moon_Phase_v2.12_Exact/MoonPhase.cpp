#include "MoonPhase.hpp"
#warning USING MOONPHASE_EXACT
double MoonPhase::_fhour(const struct tm &timeinfo)
{
    return timeinfo.tm_hour + (timeinfo.tm_min * 60 + timeinfo.tm_sec) / 3600.0;
}
static double wrap360(double deg)
{
    deg = fmod(deg, 360.0);
    return (deg < 0.0) ? deg + 360.0 : deg;
}

static int32_t wrap360(int32_t deg)
{
    deg %= 360;
    return (deg < 0) ? deg + 360 : deg;
}

// Original
//static double _Julian(int32_t year, int32_t month, const double &day)
//{
//    int32_t b, c, e;
//    b = 0;
//    if (month < 3)
//    {
//        year--;
//        month += 12;
//    }
//
//    // compensate for the Gregorian calender reform in 1582
//    // see https://en.wikipedia.org/wiki/Gregorian_calendar#Gregorian_reform
//    if (year > 1582 || (year == 1582 && month > 10) ||
//        (year == 1582 && month == 10 && day > 15))
//    {
//        int32_t a;
//        a = year / 100;
//        b = 2 - a + a / 4;
//    }
//    c = 365.25 * year;
//    e = 30.6001 * (month + 1);
//    return b + c + e + day + 1720994.5;
//}
// ChatGPT
static double _Julian(int32_t year, int32_t month, double day)
{
    int32_t a = year;
    int32_t m = month;

    if (m < 1 || m > 12)
        return std::numeric_limits<double>::quiet_NaN();

    if (m <= 2)
    {
        --a;
        m += 12;
    }

    double b = 0.0;

    // Gregorian calendar began on 1582-10-15.
    const bool gregorian =
        a > 1582 ||
        (a == 1582 && m > 10) ||
        (a == 1582 && m == 10 && day >= 15.0);

    if (gregorian)
    {
        const double century = std::floor(a / 100.0);
        b = 2.0 - century + std::floor(century / 4.0);
    }

    const double c = std::floor(365.25 * (a + 4716));
    const double e = std::floor(30.6001 * (m + 1));

    return b + c + e + day - 1524.5;
}

// Original
//static double _sun_position(const double &j)
//{
//    double n, x, e, l, dl, v;
//    int32_t i;
//    n = 360 / 365.2422 * j;
//    i = n / 360;
//    n = n - i * 360.0;
//    x = n - 3.762863;
//    x += (x < 0) ? 360 : 0;
//    x *= DEG_TO_RAD;
//    e = x;
//    do
//    {
//        dl = e - .016718 * sin(e) - x;
//        e = e - dl / (1 - .016718 * cos(e));
//    } while (fabs(dl) >= 1e-12);
//    v = 360 / PI * atan(1.01686011182 * tan(e / 2));
//    l = v + 282.596403;
//    i = l / 360;
//    l = l - i * 360.0;
//    return l;
//}

// ChatGPT
static double _sun_position(const double &j)
{
    // j is days from JD 2444238.5, the epoch used by _getPhase().
    const double jd = j + 2444238.5;
    const double T = (jd - 2451545.0) / 36525.0;

    const double L0 = wrap360(
        280.46646 + T * (36000.76983 + 0.0003032 * T));

    const double M = wrap360(
        357.52911 + T * (35999.05029 - 0.0001537 * T));

    const double Mrad = M * DEG_TO_RAD;

    const double C =
        (1.914602 - T * (0.004817 + 0.000014 * T)) * std::sin(Mrad)
        + (0.019993 - 0.000101 * T) * std::sin(2.0 * Mrad)
        + 0.000289 * std::sin(3.0 * Mrad);

    const double trueLongitude = L0 + C;
    const double omega = (125.04 - 1934.136 * T) * DEG_TO_RAD;

    // Apparent geocentric ecliptic longitude, degrees.
    return wrap360(trueLongitude - 0.00569 - 0.00478 * std::sin(omega));
}

// Original
//static double _moon_position(const double &j, const double &ls)
//{
//    double ms, l, mm, ev, sms, ae, ec;
//    int32_t i;
//    ms = 0.985647332099 * j - 3.762863;
//    ms += (ms < 0) ? 360.0 : 0;
//    l = 13.176396 * j + 64.975464;
//    i = l / 360;
//    l = l - i * 360.0;
//    l += (l < 0) ? 360 : 0;
//    mm = l - 0.1114041 * j - 349.383063;
//    i = mm / 360;
//    mm -= i * 360.0;
//    ev = 1.2739 * sin((2 * (l - ls) - mm) * DEG_TO_RAD);
//    sms = sin(ms * DEG_TO_RAD);
//    ae = 0.1858 * sms;
//    mm += ev - ae - 0.37 * sms;
//    ec = 6.2886 * sin(mm * DEG_TO_RAD);
//    l += ev + ec - ae + 0.214 * sin(2 * mm * DEG_TO_RAD);
//    l = 0.6583 * sin(2 * (l - ls) * DEG_TO_RAD) + l;
//    return l;
//}

// ChatGPT
static double _moon_position(const double &j, const double &ls)
{
    // Meeus-style periodic series for geocentric lunar ecliptic longitude.
    // Coefficients are in 1e-6 degrees.
    struct Term
    {
        int8_t d, m, mp, f;
        int32_t longitude;
    };

    static constexpr Term terms[] = {
        { 0,  0,  1,  0,  6288774},
        { 2,  0, -1,  0,  1274027},
        { 2,  0,  0,  0,   658314},
        { 0,  0,  2,  0,   213618},
        { 0,  1,  0,  0,  -185116},
        { 0,  0,  0,  2,  -114332},
        { 2,  0, -2,  0,    58793},
        { 2, -1, -1,  0,    57066},
        { 2,  0,  1,  0,    53322},
        { 2, -1,  0,  0,    45758},
        { 0,  1, -1,  0,   -40923},
        { 1,  0,  0,  0,   -34720},
        { 0,  1,  1,  0,   -30383},
        { 2,  0,  0, -2,    15327},
        { 0,  0,  1,  2,   -12528},
        { 0,  0,  1, -2,    10980},
        { 4,  0, -1,  0,    10675},
        { 0,  0,  3,  0,    10034},
        { 4,  0, -2,  0,     8548},
        { 2,  1, -1,  0,    -7888},
        { 2,  1,  0,  0,    -6766},
        { 1,  0, -1,  0,    -5163},
        { 1,  1,  0,  0,     4987},
        { 2, -1,  1,  0,     4036},
        { 2,  0,  2,  0,     3994},
        { 4,  0,  0,  0,     3861},
        { 2,  0, -3,  0,     3665},
        { 0,  1, -2,  0,    -2689},
        { 2,  0, -1,  2,    -2602},
        { 2, -1, -2,  0,     2390},
    };

    const double jd = j + 2444238.5;
    const double T = (jd - 2451545.0) / 36525.0;
    const double T2 = T * T;
    const double T3 = T2 * T;
    const double T4 = T3 * T;

    const double Lp = wrap360(
        218.3164477 + 481267.88123421 * T
        - 0.0015786 * T2 + T3 / 538841.0 - T4 / 65194000.0);

    const double D = wrap360(
        297.8501921 + 445267.1114034 * T
        - 0.0018819 * T2 + T3 / 545868.0 - T4 / 113065000.0);

    const double M = wrap360(
        357.5291092 + 35999.0502909 * T
        - 0.0001536 * T2 + T3 / 24490000.0);

    const double Mp = wrap360(
        134.9633964 + 477198.8675055 * T
        + 0.0087414 * T2 + T3 / 69699.0 - T4 / 14712000.0);

    const double F = wrap360(
        93.2720950 + 483202.0175233 * T
        - 0.0036539 * T2 - T3 / 3526000.0 + T4 / 863310000.0);

    const double E = 1.0 - 0.002516 * T - 0.0000074 * T2;

    double sum = 0.0;
    for (const Term &term : terms)
    {
        const double arg =
            (term.d * D + term.m * M + term.mp * Mp + term.f * F)
            * DEG_TO_RAD;

        double factor = 1.0;
        if (std::abs(term.m) == 1)
            factor = E;
        else if (std::abs(term.m) == 2)
            factor = E * E;

        sum += factor * term.longitude * std::sin(arg);
    }

    const double A1 = (119.75 + 131.849 * T) * DEG_TO_RAD;
    const double A2 = (53.09 + 479264.290 * T) * DEG_TO_RAD;

    // Small additive terms from the lunar longitude series.
    sum += 3958.0 * std::sin(A1)
         + 1962.0 * std::sin((Lp - F) * DEG_TO_RAD)
         + 318.0 * std::sin(A2);

    return wrap360(Lp + sum / 1000000.0);
}

//moonData_t MoonPhase::_getPhase(const int32_t year, const int32_t month, 
//                                const int32_t day, const double &hour)
//{
//    /*
//      Calculates the phase of the moon at the given epoch.
//      returns the moon phase angle as an int (0-360)
//      returns the amount that is lit as a real number (0.0-1.0)
//    */
//    const double j{_Julian(year, month, (double)day + hour / 24.0) - 2444238.5};
//    const double ls{_sun_position(j)};
//    const double lm{_moon_position(j, ls)};
//    double angle = lm - ls;
//    int32_t angleDeg = wrap360(std::lround(angle));
//    return {angleDeg, (1.0 - cos((lm - ls) * DEG_TO_RAD)) / 2};
//}

moonData_t MoonPhase::_getPhase(const int32_t year,
                                const int32_t month,
                                const int32_t day,
                                const double &hour)
{
    /*
      Calculates the phase of the moon at the given epoch.

      angleDeg   = rounded phase angle (0-359)
      angleExact = exact phase angle (0.0-360.0)
      amountLit  = illuminated fraction (0.0-1.0)
    */

    const double j{
        _Julian(year, month, (double)day + hour / 24.0) - 2444238.5
    };

    const double ls{_sun_position(j)};
    const double lm{_moon_position(j, ls)};

    const double angle = lm - ls;

    double angleExact = wrap360(angle);
    int32_t angleDeg  = wrap360(std::lround(angle));

    return {
        angleDeg,
        angleExact,
        (1.0 - cos(angle * DEG_TO_RAD)) / 2
    };
}
