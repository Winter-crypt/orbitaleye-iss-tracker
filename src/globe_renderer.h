#pragma once
#include <TFT_eSPI.h>
#include <pgmspace.h>
#include "config.h"
#include "globe_data.h"
#include "worldmap.h"

// =============================================================
//  OrbitalEye CYD — globe_renderer.h
//  Mappa 2D equirettangolare con bitmap 1-bit 240x210 nativa.
//  No stretching verticale — Europa e continenti corretti.
//  Rendering a scanline con run-length — veloce.
//  Traccia orbita ISS: ultimi 90 punti con fade.
// =============================================================

// Area mappa sul display
#define MAP_X0   0
#define MAP_Y0   (TABBAR_H + 2)
#define MAP_W    240
#define MAP_H    (SCREEN_H - MAP_Y0)    // 210px

// Colori
#define C_OCEAN_DEEP  tft.color565(  5,  25,  65)
#define C_OCEAN_MID   tft.color565( 10,  45, 100)
#define C_LAND_LOW    tft.color565( 50,  90,  45)
#define C_LAND_HIGH   tft.color565( 80, 125,  65)
#define C_GRID        tft.color565( 18,  40,  80)
#define C_EQUATOR     tft.color565( 30,  65, 105)
#define C_TRACK       tft.color565(  0, 200, 180)
#define C_ISS         TFT_YELLOW
#define C_ISS_RING    TFT_ORANGE
#define C_BORDER      tft.color565( 45,  75, 115)

// Legge 1 bit dalla worldmap bitmap in PROGMEM
// WMAP_W=240, WMAP_H=210 — uguale all'area display: mapping 1:1
static inline bool wmapIsLand(int bx, int by) {
    if (bx < 0 || bx >= WMAP_W || by < 0 || by >= WMAP_H) return false;
    int idx = by * WMAP_W + bx;
    uint8_t byte = pgm_read_byte(&WORLD_MAP[idx >> 3]);
    return (byte >> (7 - (idx & 7))) & 1;
}

// Mapping pixel display → bitmap: ora 1:1 perché entrambi 240x210
static inline bool mapPixelIsLand(int px, int py) {
    int bx = px - MAP_X0;
    int by = py - MAP_Y0;
    return wmapIsLand(bx, by);
}

// Converti lat/lon → pixel display
static void geoToPixel(float lat, float lon, int16_t &px, int16_t &py) {
    while (lon >  180.f) lon -= 360.f;
    while (lon < -180.f) lon += 360.f;
    px = MAP_X0 + (int16_t)((lon + 180.f) / 360.f * MAP_W);
    py = MAP_Y0 + (int16_t)((90.f - lat) / 180.f * MAP_H);
    px = constrain(px, MAP_X0, MAP_X0 + MAP_W - 1);
    py = constrain(py, MAP_Y0, MAP_Y0 + MAP_H - 1);
}

// Traccia orbita
#define TRACK_LEN 90
static float    _trkLat[TRACK_LEN];
static float    _trkLon[TRACK_LEN];
static uint8_t  _trkHead  = 0;
static uint8_t  _trkCount = 0;
static uint32_t _trkMs    = 0;

// Stato rendering
static bool     _bgReady  = false;
static bool     _issVis   = true;
static uint32_t _blinkMs  = 0;
static float    _prevLat  = 999.f;
static float    _prevLon  = 999.f;
static uint32_t _fullMs   = 0;

// =============================================================
//  Sfondo: oceano + terra dalla bitmap + griglia
// =============================================================
static void drawBackground(TFT_eSPI& tft) {
    tft.startWrite();

    for (int py = MAP_Y0; py < MAP_Y0 + MAP_H; py++) {
        int by = py - MAP_Y0;  // riga bitmap = riga display (1:1)

        bool curLand = wmapIsLand(0, by);
        int runStart = MAP_X0;

        for (int px = MAP_X0 + 1; px <= MAP_X0 + MAP_W; px++) {
            int bx = px - MAP_X0;
            bool land = (bx < WMAP_W) ? wmapIsLand(bx, by) : !curLand;

            if (land != curLand) {
                float lat = 90.f - (float)(py - MAP_Y0) / MAP_H * 180.f;
                uint16_t col;
                if (curLand) {
                    float s = 0.7f + 0.3f * (1.f - fabsf(lat) / 90.f);
                    col = tft.color565(
                        (uint8_t)(55 * s), (uint8_t)(95 * s), (uint8_t)(42 * s));
                } else {
                    float s = 0.6f + 0.4f * (1.f - fabsf(lat) / 90.f);
                    col = tft.color565(
                        (uint8_t)(5 * s), (uint8_t)(35 * s), (uint8_t)(85 * s));
                }
                tft.drawFastHLine(runStart, py, px - runStart, col);
                runStart = px;
                curLand  = land;
            }
        }
    }

    // Griglia lon ogni 30°
    for (int lon = -180; lon <= 180; lon += 30) {
        int16_t gx, gy;
        geoToPixel(0, lon, gx, gy);
        uint16_t col = (lon == 0) ? C_EQUATOR : C_GRID;
        tft.drawFastVLine(gx, MAP_Y0, MAP_H, col);
    }
    // Griglia lat ogni 30°
    for (int lat = -90; lat <= 90; lat += 30) {
        int16_t gx, gy;
        geoToPixel(lat, 0, gx, gy);
        uint16_t col = (lat == 0) ? C_EQUATOR : C_GRID;
        tft.drawFastHLine(MAP_X0, gy, MAP_W, col);
    }

    tft.drawRect(MAP_X0, MAP_Y0, MAP_W, MAP_H, C_BORDER);
    tft.endWrite();
}

// =============================================================
//  Traccia orbita con fade
// =============================================================
static void drawTrack(TFT_eSPI& tft) {
    if (_trkCount < 2) return;
    tft.startWrite();
    for (uint8_t i = 1; i < _trkCount; i++) {
        uint8_t cur  = (_trkHead - i     + TRACK_LEN) % TRACK_LEN;
        uint8_t prev = (_trkHead - i - 1 + TRACK_LEN) % TRACK_LEN;
        if (fabsf(_trkLon[cur] - _trkLon[prev]) > 180.f) continue;
        int16_t x1, y1, x2, y2;
        geoToPixel(_trkLat[prev], _trkLon[prev], x1, y1);
        geoToPixel(_trkLat[cur],  _trkLon[cur],  x2, y2);
        float age = (float)i / _trkCount;
        uint8_t b = (uint8_t)(210 * (1.f - age * 0.75f));
        tft.drawLine(x1, y1, x2, y2, tft.color565(0, b, (uint8_t)(b * 0.85f)));
    }
    tft.endWrite();
}

// =============================================================
//  Cancella vecchio marker ISS (ripristina sfondo locale 1:1)
// =============================================================
static void eraseISSMarker(TFT_eSPI& tft, float lat, float lon) {
    int16_t ox, oy;
    geoToPixel(lat, lon, ox, oy);
    const int R = 10;
    tft.startWrite();
    for (int dy = -R; dy <= R; dy++) {
        for (int dx = -R; dx <= R; dx++) {
            int px = ox + dx, py = oy + dy;
            if (px < MAP_X0 || px >= MAP_X0 + MAP_W) continue;
            if (py < MAP_Y0 || py >= MAP_Y0 + MAP_H) continue;
            // 1:1 mapping
            bool land = wmapIsLand(px - MAP_X0, py - MAP_Y0);
            float plat = 90.f - (float)(py - MAP_Y0) / MAP_H * 180.f;
            float plon = (float)(px - MAP_X0) / MAP_W * 360.f - 180.f;
            uint16_t col;
            bool onGridLon = false, onGridLat = false;
            for (int g = -180; g <= 180; g += 30) {
                int16_t gx, gy; geoToPixel(0, g, gx, gy);
                if (px == gx) { onGridLon = true; break; }
            }
            for (int g = -90; g <= 90; g += 30) {
                int16_t gx, gy; geoToPixel(g, 0, gx, gy);
                if (py == gy) { onGridLat = true; break; }
            }
            if (onGridLon || onGridLat) {
                col = (onGridLon && (int)plon == 0) || (onGridLat && (int)plat == 0)
                    ? C_EQUATOR : C_GRID;
            } else if (land) {
                float s = 0.7f + 0.3f * (1.f - fabsf(plat) / 90.f);
                col = tft.color565((uint8_t)(55*s),(uint8_t)(95*s),(uint8_t)(42*s));
            } else {
                float s = 0.6f + 0.4f * (1.f - fabsf(plat) / 90.f);
                col = tft.color565((uint8_t)(5*s),(uint8_t)(35*s),(uint8_t)(85*s));
            }
            tft.drawPixel(px, py, col);
        }
    }
    tft.endWrite();
}

// =============================================================
//  renderGlobe — entry point
// =============================================================
void renderGlobe(TFT_eSPI& tft, float issLat, float issLon) {
    uint32_t now = millis();

    if (now - _blinkMs > 600) { _issVis = !_issVis; _blinkMs = now; }

    if (now - _trkMs > 10000) {
        _trkLat[_trkHead] = issLat;
        _trkLon[_trkHead] = issLon;
        _trkHead  = (_trkHead + 1) % TRACK_LEN;
        if (_trkCount < TRACK_LEN) _trkCount++;
        _trkMs = now;
    }

    bool fullRedraw = !_bgReady || (now - _fullMs > 8000);
    if (fullRedraw) {
        drawBackground(tft);
        drawTrack(tft);
        _bgReady = true;
        _fullMs  = now;
        _prevLat = 999.f;
    } else {
        if (_prevLat != 999.f) {
            eraseISSMarker(tft, _prevLat, _prevLon);
            drawTrack(tft);
        }
    }

    int16_t ix, iy;
    geoToPixel(issLat, issLon, ix, iy);

    tft.startWrite();
    if (_issVis) {
        tft.fillCircle(ix, iy, 4, C_ISS);
        tft.drawCircle(ix, iy, 7, C_ISS_RING);
        tft.drawCircle(ix, iy, 8, tft.color565(180, 80, 0));
    } else {
        tft.drawCircle(ix, iy, 7, C_ISS_RING);
    }
    tft.endWrite();

    _prevLat = issLat;
    _prevLon = issLon;
}

void mapInvalidate() {
    _bgReady = false;
    _prevLat = 999.f;
}