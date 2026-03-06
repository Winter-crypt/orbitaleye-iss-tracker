#pragma once
#include <Arduino.h>

static bool isLand(float lat, float lon) {
    while (lon >  180.0f) lon -= 360.0f;
    while (lon < -180.0f) lon += 360.0f;

    if (lat < -60.0f) return true;
    if (lat > 60.0f && lat < 84.0f && lon > -55.0f && lon < -15.0f) return true;
    if (lat > 25.0f && lat < 72.0f && lon > -168.0f && lon < -52.0f) {
        if (lat > 25.0f && lat < 30.0f && lon > -90.0f && lon < -80.0f) return false;
        return true;
    }
    if (lat > 15.0f && lat < 25.0f && lon > -90.0f && lon < -75.0f) return true;
    if (lat > -56.0f && lat < 13.0f && lon > -82.0f && lon < -34.0f) return true;
    if (lat > 35.0f && lat < 72.0f && lon > -10.0f && lon < 40.0f) {
        if (lat > 35.0f && lat < 46.0f && lon > 0.0f && lon < 36.0f)
            if (lat < 38.5f && lon > 10.0f) return false;
        return true;
    }
    if (lat > 63.0f && lat < 67.0f && lon > -24.0f && lon < -13.0f) return true;
    if (lat > 55.0f && lat < 72.0f && lon > 14.0f  && lon < 32.0f)  return true;
    if (lat > -35.0f && lat < 37.0f && lon > -17.0f && lon < 52.0f) {
        if (lat < 5.0f && lon > 42.0f) return false;
        return true;
    }
    if (lat > -26.0f && lat < -12.0f && lon > 43.0f && lon < 51.0f) return true;
    if (lat > 0.0f && lat < 77.0f && lon > 26.0f && lon < 180.0f) {
        if (lat < 30.0f && lon > 50.0f && lon < 60.0f)   return false;
        if (lat < 15.0f && lon > 100.0f && lon < 120.0f) return false;
        return true;
    }
    if (lat > -10.0f && lat < 0.0f  && lon > 95.0f  && lon < 142.0f) return true;
    if (lat > 12.0f  && lat < 30.0f && lon > 43.0f  && lon < 60.0f)  return true;
    if (lat > 31.0f  && lat < 45.0f && lon > 129.0f && lon < 145.0f) return true;
    if (lat > -8.0f  && lat < 20.0f && lon > 95.0f  && lon < 127.0f) return true;
    if (lat > -44.0f && lat < -10.0f && lon > 113.0f && lon < 154.0f) return true;
    if (lat > -47.0f && lat < -34.0f && lon > 166.0f && lon < 178.0f) return true;
    if (lat > 50.0f  && lat < 72.0f && lon > 140.0f && lon < 180.0f) return true;
    if (lat > 55.0f  && lat < 72.0f && lon > -170.0f && lon < -130.0f) return true;
    return false;
}

struct GeoZone {
    const char* name;
    const char* hemisphere;
};

static GeoZone getGeoZone(float lat, float lon) {
    while (lon >  180.0f) lon -= 360.0f;
    while (lon < -180.0f) lon += 360.0f;
    const char* hem = (lat >= 0) ? "N" : "S";

    if (isLand(lat, lon)) {
        if (lat < -60.0f)                                                       return {"Antarctica",      hem};
        if (lat > 60.0f  && lon > -55.0f  && lon < -15.0f)                     return {"Greenland",       hem};
        if (lat > 25.0f  && lon > -168.0f && lon < -52.0f)                     return {"North America",   hem};
        if (lat > 15.0f  && lon > -90.0f  && lon < -75.0f)                     return {"Central America", hem};
        if (lat < 13.0f  && lon > -82.0f  && lon < -34.0f)                     return {"South America",   hem};
        if (lat > 35.0f  && lon > -10.0f  && lon < 40.0f)                      return {"Europe",          hem};
        if (lat > 63.0f  && lon > -24.0f  && lon < -13.0f)                     return {"Iceland",         hem};
        if (lat > -35.0f && lon > -17.0f  && lon < 52.0f)                      return {"Africa",          hem};
        if (lat > -26.0f && lat < -12.0f  && lon > 43.0f  && lon < 51.0f)     return {"Madagascar",      hem};
        if (lat > 12.0f  && lon > 43.0f   && lon < 60.0f)                      return {"Arabian Pen.",    hem};
        if (lat > -44.0f && lat < -10.0f  && lon > 113.0f && lon < 154.0f)    return {"Australia",       hem};
        if (lat > -47.0f && lat < -34.0f  && lon > 166.0f)                     return {"New Zealand",     hem};
        if (lat > 31.0f  && lon > 129.0f  && lon < 145.0f)                     return {"Japan",           hem};
        if (lat > 0.0f   && lon > 26.0f   && lon < 180.0f)                     return {"Asia",            hem};
        return {"Land", hem};
    }

    if (lon > -70.0f  && lon < 20.0f  && lat > -60.0f && lat < 80.0f) return {"Atlantic Ocean", hem};
    if (lon > 20.0f   && lon < 147.0f && lat > -60.0f && lat < 30.0f) return {"Indian Ocean",   hem};
    if (lat > 65.0f)                                                    return {"Arctic Ocean",   hem};
    if (lat < -60.0f)                                                   return {"Southern Ocean", hem};
    return {"Pacific Ocean", hem};
}