#pragma once
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <stdint.h>
#include "config.h"
#include "globe_data.h"

// =============================================================
//  OrbitalEye CYD — ui.h  v2.2
//  Fix flickering:
//  - DATA: parti statiche (box, label) disegnate una volta sola;
//          solo i valori numerici vengono aggiornati se cambiano
//  - SYS:  stessa strategia; barre aggiornate in delta (no clear)
//  - _barDelta: aggiorna solo la porzione modificata della barra
//  - _tabSwitchCount: trigger full-redraw al cambio di tab
//  Firme tutte invariate — main.cpp non richiede modifiche.
// =============================================================

#define UI_BG TFT_BLACK
#define BODY_Y (TABBAR_H + 2)
#define BODY_H (SCREEN_H - TABBAR_H - 2)

static const char *TAB_NAMES[] = {"GLOBE", "DATA", "WHERE", "CREW", "SYS", "PASS"};
static const uint8_t TAB_N = 6;
static uint8_t _lastTab = 255;
static uint8_t _tabSwitchCount = 0; // incrementato ad ogni cambio tab

// ── Palette ───────────────────────────────────────────────────
#define C_ACCENT tft.color565(0, 180, 220)
#define C_LABEL tft.color565(110, 110, 110)
#define C_SEP tft.color565(35, 35, 35)
#define C_SEP2 tft.color565(25, 55, 65)
#define C_GREEN tft.color565(60, 200, 80)
#define C_ORANGE tft.color565(220, 140, 40)
#define C_RED tft.color565(220, 60, 60)
#define C_TABDIM tft.color565(75, 75, 75)
#define C_TABBG tft.color565(18, 18, 18)
#define C_BOXBG tft.color565(10, 28, 36)
#define C_WHITE tft.color565(230, 230, 230)

// ── Helper: rettangolo con bordo colorato ─────────────────────
static void _box(TFT_eSPI &tft, int x, int y, int w, int h,
                 uint16_t border, uint16_t bg = 0)
{
    if (bg)
        tft.fillRect(x + 1, y + 1, w - 2, h - 2, bg);
    tft.drawRect(x, y, w, h, border);
}

// ── Helper: barra intera (primo disegno) ─────────────────────
static void _bar(TFT_eSPI &tft, int x, int y, int w, int h,
                 int pct, uint16_t col)
{
    tft.fillRect(x, y, w, h, tft.color565(20, 20, 20));
    tft.drawRect(x, y, w, h, tft.color565(50, 50, 50));
    int fw = (int)((float)(w - 2) * pct / 100.0f);
    if (fw > 0)
        tft.fillRect(x + 1, y + 1, fw, h - 2, col);
}

// ── Helper: aggiorna solo la delta della barra (no flicker) ──
//   oldPct = -1 forza ridisegno completo
static void _barDelta(TFT_eSPI &tft, int x, int y, int w, int h,
                      int newPct, int oldPct, uint16_t col)
{
    if (newPct == oldPct)
        return;
    if (oldPct < 0)
    {
        _bar(tft, x, y, w, h, newPct, col);
        return;
    }

    int newFw = (int)((float)(w - 2) * newPct / 100.0f);
    int oldFw = (int)((float)(w - 2) * oldPct / 100.0f);
    if (newFw > oldFw)
        tft.fillRect(x + 1 + oldFw, y + 1, newFw - oldFw, h - 2, col);
    else
        tft.fillRect(x + 1 + newFw, y + 1, oldFw - newFw, h - 2,
                     tft.color565(20, 20, 20));
}

// ── Helper: riga label + valore allineata ─────────────────────
static void _row(TFT_eSPI &tft, int x, int y,
                 const char *lbl, const char *val,
                 uint16_t vc = 0xFFFF)
{
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(x, y);
    tft.print(lbl);
    tft.setTextColor(vc, UI_BG);
    tft.setCursor(x + 56, y);
    char b[22];
    snprintf(b, sizeof(b), "%-16s", val);
    tft.print(b);
}

// =============================================================
//  TAB BAR
// =============================================================
void ui_drawTabBar(TFT_eSPI &tft, uint8_t active)
{
    if (_lastTab == active)
        return;
    _lastTab = active;
    _tabSwitchCount++; // segnala cambio a tutti i tab
    tft.setTextFont(1);
    tft.setTextSize(1);
    const int tw = SCREEN_W / TAB_N;
    for (uint8_t i = 0; i < TAB_N; i++)
    {
        int x = i * tw;
        bool a = (i == active);
        tft.fillRect(x, 0, tw, TABBAR_H, a ? C_ACCENT : C_TABBG);
        if (a)
            tft.fillRect(x, TABBAR_H - 2, tw, 2, C_WHITE);
        int tx = x + (tw - (int)strlen(TAB_NAMES[i]) * 6) / 2;
        int ty = (TABBAR_H - 8) / 2;
        tft.setTextColor(a ? UI_BG : C_TABDIM, a ? C_ACCENT : C_TABBG);
        tft.setCursor(tx, ty);
        tft.print(TAB_NAMES[i]);
        if (i < TAB_N - 1)
            tft.drawFastVLine(x + tw - 1, 0, TABBAR_H, C_SEP);
    }
    tft.drawFastHLine(0, TABBAR_H, SCREEN_W, C_ACCENT);
}

// =============================================================
//  GLOBE SIDEBAR
// =============================================================
void ui_drawGlobeSidebar(TFT_eSPI &tft, float lat, float lon, float alt)
{
    tft.setTextFont(2);
    tft.setTextSize(1);
    int x = SIDEBAR_X, y = BODY_Y + 6;
    char b[14];
    tft.setTextColor(C_ACCENT, UI_BG);
    tft.setCursor(x, y);
    tft.print("ISS");
    y += 16;
    tft.drawFastHLine(x, y, 74, C_SEP2);
    y += 4;
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(x, y);
    tft.print("LAT");
    y += 14;
    snprintf(b, sizeof(b), "%+.1f", lat);
    tft.setTextColor(C_WHITE, UI_BG);
    tft.setCursor(x, y);
    tft.print(b);
    tft.print("  ");
    y += 18;
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(x, y);
    tft.print("LON");
    y += 14;
    snprintf(b, sizeof(b), "%+.1f", lon);
    tft.setTextColor(C_WHITE, UI_BG);
    tft.setCursor(x, y);
    tft.print(b);
    tft.print("  ");
    y += 18;
    tft.drawFastHLine(x, y, 74, C_SEP2);
    y += 4;
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(x, y);
    tft.print("ALT");
    y += 14;
    snprintf(b, sizeof(b), "%.0f", alt);
    tft.setTextColor(C_GREEN, UI_BG);
    tft.setCursor(x, y);
    tft.print(b);
    tft.print("km");
    y += 18;
    tft.drawFastHLine(x, y, 74, C_SEP2);
    y += 4;
    GeoZone gz = getGeoZone(lat, lon);
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(x, y);
    tft.print("OVER");
    y += 14;
    char zone[9];
    strncpy(zone, gz.name, 8);
    zone[8] = '\0';
    tft.setTextColor(C_ORANGE, UI_BG);
    tft.setCursor(x, y);
    tft.print(zone);
    tft.print("  ");
}

// =============================================================
//  TAB DATA — flicker-free: statica disegnata una volta,
//             aggiornati solo i numeri che cambiano
// =============================================================
void ui_drawData(TFT_eSPI &tft, float lat, float lon, float alt, float vel)
{
    static float _pLat = 1e9f, _pLon = 1e9f, _pAlt = 1e9f, _pVel = 1e9f;
    static uint8_t _mySwitchCount = 255;

    const int x = 6, y0 = BODY_Y + 4;
    const int BW = 148, BH = 44, GAP = 4;
    const int lx = x + BW + GAP;

    bool full = (_mySwitchCount != _tabSwitchCount);
    if (full)
    {
        _mySwitchCount = _tabSwitchCount;
        _pLat = _pLon = _pAlt = _pVel = 1e9f;
        tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, UI_BG);
    }

    char b[24];

    // ── Parti statiche (solo al primo disegno / cambio tab) ───
    if (full)
    {
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(C_ACCENT, UI_BG);
        tft.setCursor(x, y0);
        tft.print("ISS TELEMETRY");
        tft.drawFastHLine(x, y0 + 16, SCREEN_W - 12, C_SEP2);

        int ry = y0 + 20;
        tft.fillRect(x + 1, ry + 1, BW - 2, BH - 2, C_BOXBG);
        tft.fillRect(lx + 1, ry + 1, BW - 2, BH - 2, C_BOXBG);
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, C_BOXBG);
        tft.setCursor(x + 5, ry + 4);
        tft.print("LATITUDE");
        tft.setCursor(lx + 5, ry + 4);
        tft.print("LONGITUDE");
        tft.setCursor(x + BW - 16, ry + 30);
        tft.print("deg");
        tft.setCursor(lx + BW - 16, ry + 30);
        tft.print("deg");

        ry += BH + GAP;
        tft.fillRect(x + 1, ry + 1, BW - 2, BH - 2, C_BOXBG);
        tft.fillRect(lx + 1, ry + 1, BW - 2, BH - 2, C_BOXBG);
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, C_BOXBG);
        tft.setCursor(x + 5, ry + 4);
        tft.print("ALTITUDE");
        tft.setCursor(lx + 5, ry + 4);
        tft.print("VELOCITY");
        tft.setCursor(x + BW - 12, ry + 30);
        tft.print("km");
        tft.setCursor(lx + BW - 22, ry + 30);
        tft.print("km/s");

        int ry3 = y0 + 20 + (BH + GAP) * 2 + 8;
        tft.drawFastHLine(x, ry3 - 2, SCREEN_W - 12, C_SEP);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(x, ry3);
        tft.print("INC");
        tft.setTextColor(C_WHITE, UI_BG);
        tft.setCursor(x + 26, ry3);
        tft.print("51.6\xF7");
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(x + 88, ry3);
        tft.print("PER");
        tft.setTextColor(C_WHITE, UI_BG);
        tft.setCursor(x + 114, ry3);
        tft.print("92.9m");
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(x + 180, ry3);
        tft.print("APO");
        tft.setTextColor(C_WHITE, UI_BG);
        tft.setCursor(x + 206, ry3);
        tft.print("408km");
    }

    // ── Aggiorna solo i valori cambiati ───────────────────────
    int ry1 = y0 + 20;
    int ry2 = ry1 + BH + GAP;

    if (lat != _pLat)
    {
        _pLat = lat;
        uint16_t bc = lat >= 0 ? C_GREEN : C_ORANGE;
        tft.drawRect(x, ry1, BW, BH, bc);
        tft.fillRect(x + 5, ry1 + 17, BW - 25, 21, C_BOXBG);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(bc, C_BOXBG);
        snprintf(b, sizeof(b), "%+.2f", lat);
        tft.setCursor(x + 5, ry1 + 18);
        tft.print(b);
    }
    if (lon != _pLon)
    {
        _pLon = lon;
        uint16_t bc = lon >= 0 ? C_GREEN : C_ORANGE;
        tft.drawRect(lx, ry1, BW, BH, bc);
        tft.fillRect(lx + 5, ry1 + 17, BW - 25, 21, C_BOXBG);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(bc, C_BOXBG);
        snprintf(b, sizeof(b), "%+.2f", lon);
        tft.setCursor(lx + 5, ry1 + 18);
        tft.print(b);
    }
    if (alt != _pAlt)
    {
        _pAlt = alt;
        tft.drawRect(x, ry2, BW, BH, C_ACCENT);
        tft.fillRect(x + 5, ry2 + 17, BW - 20, 21, C_BOXBG);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(C_ACCENT, C_BOXBG);
        snprintf(b, sizeof(b), "%.1f", alt);
        tft.setCursor(x + 5, ry2 + 18);
        tft.print(b);
    }
    if (vel != _pVel)
    {
        _pVel = vel;
        tft.drawRect(lx, ry2, BW, BH, C_ORANGE);
        tft.fillRect(lx + 5, ry2 + 17, BW - 30, 21, C_BOXBG);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(C_ORANGE, C_BOXBG);
        snprintf(b, sizeof(b), "%.2f", vel);
        tft.setCursor(lx + 5, ry2 + 18);
        tft.print(b);
    }
}

// =============================================================
//  TAB WHERE — bussola grafica + testo più grande
// =============================================================
static void _drawCompass(TFT_eSPI &tft, int cx, int cy, int r,
                         float lat, float lon)
{
    tft.drawCircle(cx, cy, r, C_SEP2);
    tft.drawCircle(cx, cy, r + 1, tft.color565(20, 20, 20));
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(cx - 3, cy - r - 9);
    tft.print("N");
    tft.setCursor(cx - 3, cy + r + 2);
    tft.print("S");
    tft.setCursor(cx + r + 3, cy - 4);
    tft.print("E");
    tft.setCursor(cx - r - 9, cy - 4);
    tft.print("W");
    for (int a = 0; a < 360; a += 45)
    {
        float rad = a * M_PI / 180.0f;
        int x1 = cx + (int)(cosf(rad) * (r - 3));
        int y1 = cy + (int)(sinf(rad) * (r - 3));
        int x2 = cx + (int)(cosf(rad) * r);
        int y2 = cy + (int)(sinf(rad) * r);
        tft.drawLine(x1, y1, x2, y2, C_SEP2);
    }
    float nx = (lon / 180.0f) * r * 0.8f;
    float ny = -(lat / 90.0f) * r * 0.8f;
    tft.fillCircle(cx + (int)nx, cy + (int)ny, 3, C_ACCENT);
    tft.drawCircle(cx + (int)nx, cy + (int)ny, 5, tft.color565(0, 80, 100));
}

void ui_drawWhere(TFT_eSPI &tft, float lat, float lon, const char *zoneName)
{
    const int cx = SCREEN_W / 2;
    char b[36];
    int y = BODY_Y + 6;

    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.setTextColor(C_WHITE, UI_BG);
    int zx = cx - ((int)strlen(zoneName) * 14) / 2;
    if (zx < 4)
        zx = 4;
    tft.fillRect(0, y, SCREEN_W, 28, UI_BG);
    tft.setCursor(zx, y);
    tft.print(zoneName);
    y += 32;
    tft.drawFastHLine(16, y, SCREEN_W - 32, C_SEP2);
    y += 8;

    const int COMP_CX = 52, COMP_CY = y + 52, COMP_R = 38;
    _drawCompass(tft, COMP_CX, COMP_CY, COMP_R, lat, lon);

    int rx = COMP_CX + COMP_R + 18, ry = y + 4;
    const char *ns = lat >= 0 ? "Northern" : "Southern";
    const char *ew = lon >= 0 ? "Eastern" : "Western";
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(C_ORANGE, UI_BG);
    tft.setCursor(rx, ry);
    tft.print(ns);
    ry += 16;
    tft.setTextColor(C_ORANGE, UI_BG);
    tft.setCursor(rx, ry);
    tft.print(ew);
    ry += 22;
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(rx, ry);
    tft.print("LAT");
    ry += 14;
    snprintf(b, sizeof(b), "%+.4f", lat);
    tft.setTextColor(C_WHITE, UI_BG);
    tft.setCursor(rx, ry);
    tft.print(b);
    ry += 18;
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(rx, ry);
    tft.print("LON");
    ry += 14;
    snprintf(b, sizeof(b), "%+.4f", lon);
    tft.setTextColor(C_WHITE, UI_BG);
    tft.setCursor(rx, ry);
    tft.print(b);

    y = COMP_CY + COMP_R + 12;
    tft.drawFastHLine(16, y, SCREEN_W - 32, C_SEP);
    y += 6;
    int ld = (int)fabsf(lat), lm = (int)((fabsf(lat) - ld) * 60);
    int od = (int)fabsf(lon), om = (int)((fabsf(lon) - od) * 60);
    snprintf(b, sizeof(b), "%02d\xF7%02d'%c   %03d\xF7%02d'%c",
             ld, lm, lat >= 0 ? 'N' : 'S', od, om, lon >= 0 ? 'E' : 'W');
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(cx - ((int)strlen(b) * 7) / 2, y);
    tft.print(b);
}

// =============================================================
//  TAB CREW — card con iniziale + lista
// =============================================================
static uint8_t _ci = 0;
static uint32_t _cMs = 0;

static void _drawCrewAvatar(TFT_eSPI &tft, int cx, int cy, int r,
                            char initial, uint16_t col)
{
    tft.fillCircle(cx, cy, r, tft.color565(15, 35, 45));
    tft.drawCircle(cx, cy, r, col);
    tft.drawCircle(cx, cy, r - 1, tft.color565(0, 60, 80));
    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.setTextColor(col, tft.color565(15, 35, 45));
    char s[2] = {initial, 0};
    tft.setCursor(cx - 8, cy - 12);
    tft.print(s);
}

void ui_drawCrew(TFT_eSPI &tft, String names[], int count)
{
    if (count <= 0)
    {
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(40, SCREEN_H / 2);
        tft.print("No crew data available  ");
        return;
    }
    uint32_t now = millis();
    if (now - _cMs > 3500)
    {
        _ci = (_ci + 1) % count;
        _cMs = now;
        tft.fillRect(0, BODY_Y + 30, SCREEN_W, 70, UI_BG);
    }
    const int cx = SCREEN_W / 2;
    int y = BODY_Y + 6;
    char h[26];
    snprintf(h, sizeof(h), "ISS CREW  %d aboard", count);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(C_ACCENT, UI_BG);
    tft.setCursor(cx - ((int)strlen(h) * 7) / 2, y);
    tft.print(h);
    y += 16;
    tft.drawFastHLine(16, y, SCREEN_W - 32, C_SEP2);
    y += 6;

    const int AV_X = 36, AV_Y = y + 28, AV_R = 22;
    char init = names[_ci].length() > 0 ? names[_ci][0] : '?';
    _drawCrewAvatar(tft, AV_X, AV_Y, AV_R, init, C_ACCENT);

    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.setTextColor(C_WHITE, UI_BG);
    String nm = names[_ci];
    int sp = nm.indexOf(' ');
    String surname = (sp > 0) ? nm.substring(sp + 1) : nm;
    int nx = AV_X + AV_R + 10;
    if (surname.length() > 11)
        surname = surname.substring(0, 11);
    tft.setCursor(nx, y + 14);
    tft.print(surname + "  ");

    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(C_LABEL, UI_BG);
    String firstname = (sp > 0) ? nm.substring(0, sp) : "";
    tft.setCursor(nx, y + 42);
    tft.print(firstname + "  ");
    y += AV_R * 2 + 14;

    int dg = 16, dr = 4, dtot = count * dg - (dg - dr * 2), dx = cx - dtot / 2 + dr;
    for (int i = 0; i < count && i < 10; i++)
    {
        if (i == _ci)
            tft.fillCircle(dx, y + dr, dr, C_ACCENT);
        else
        {
            tft.fillCircle(dx, y + dr, dr, C_SEP);
            tft.drawCircle(dx, y + dr, dr, C_LABEL);
        }
        dx += dg;
    }
    y += dr * 2 + 10;
    tft.drawFastHLine(16, y, SCREEN_W - 32, C_SEP);
    y += 6;
    tft.setTextFont(1);
    tft.setTextSize(1);
    for (int i = 0; i < count && y < SCREEN_H - 8; i++)
    {
        tft.setTextColor(i == _ci ? C_ACCENT : C_LABEL, UI_BG);
        tft.setCursor(16, y);
        char r[28];
        snprintf(r, sizeof(r), "%-26s", names[i].c_str());
        tft.print(r);
        y += 13;
    }
}

// =============================================================
//  TAB SYS — flicker-free: statica una volta, barre in delta
// =============================================================
void ui_drawSys(TFT_eSPI &tft, uint32_t upSec, uint32_t orbits,
                int32_t rssi, bool valid)
{
    static uint8_t _mySwitchCount = 255;
    static int _pRssiPct = -1, _pHeapPct = -1;
    static int32_t _pRssi = 1;
    static bool _pValid = false;
    static uint32_t _pUpSec = 0xFFFFFFFF;
    static uint32_t _pOrbits = 0xFFFFFFFF;

    const int x = 8;
    char b[28];

    bool full = (_mySwitchCount != _tabSwitchCount);
    if (full)
    {
        _mySwitchCount = _tabSwitchCount;
        _pRssiPct = _pHeapPct = -1;
        _pRssi = 1;
        _pValid = !valid;
        _pUpSec = _pOrbits = 0xFFFFFFFF;
        tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, UI_BG);

        int y = BODY_Y + 6;
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(C_ACCENT, UI_BG);
        tft.setCursor(x, y);
        tft.print("SYSTEM STATUS");
        tft.drawFastHLine(x, y + 16, SCREEN_W - 16, C_SEP2);
        y += 22;
        _row(tft, x, y, "FW    ", "OrbitalEye v2.2");
        y += 17;
        _row(tft, x, y, "DISP  ", "ILI9341 320x240");
        y += 17;
        _row(tft, x, y, "TOUCH ", "XPT2046 HSPI");
        y += 17;
        tft.drawFastHLine(x, y, SCREEN_W - 16, C_SEP);
        // separatore sotto orbits (disegnato dopo)
    }

    // Posizione base delle righe dinamiche
    // (dopo titolo 22px + 3 righe statiche 17px + sep 6px = 22+51+6 = 79)
    int yUp = BODY_Y + 6 + 22 + 51 + 6;
    int yOrb = yUp + 17;
    int ySep = yOrb + 17;
    int yRsi = ySep + 6;
    int yRsiBar = yRsi + 15;
    int yHp = yRsiBar + 14;
    int yHpBar = yHp + 15;
    int ySep2 = yHpBar + 14;
    int yDat = ySep2 + 6;

    if (full)
    {
        tft.drawFastHLine(x, ySep, SCREEN_W - 16, C_SEP);
        tft.drawFastHLine(x, ySep2, SCREEN_W - 16, C_SEP);
        // Label RSSI e HEAP statiche
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(x, yRsi);
        tft.print("RSSI  ");
        tft.setCursor(x, yHp);
        tft.print("HEAP  ");
    }

    // ── Uptime ────────────────────────────────────────────────
    if (upSec != _pUpSec)
    {
        _pUpSec = upSec;
        snprintf(b, sizeof(b), "%02u:%02u:%02u",
                 upSec / 3600, (upSec % 3600) / 60, upSec % 60);
        _row(tft, x, yUp, "UP    ", b);
    }

    // ── Orbite ────────────────────────────────────────────────
    if (orbits != _pOrbits)
    {
        _pOrbits = orbits;
        snprintf(b, sizeof(b), "%u est.", orbits);
        _row(tft, x, yOrb, "ORBITS", b);
    }

    // ── RSSI: testo + barra delta ─────────────────────────────
    int rssiPct = (int)constrain(((rssi + 100) * 100) / 60, 0, 100);
    uint16_t rc = rssi > -60 ? C_GREEN : rssi > -75 ? C_ORANGE
                                                    : C_RED;

    if (rssi != _pRssi)
    {
        _pRssi = rssi;
        snprintf(b, sizeof(b), "%d dBm ", rssi);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(rc, UI_BG);
        tft.setCursor(x + 56, yRsi);
        tft.print(b);
    }
    if (rssiPct != _pRssiPct)
    {
        _barDelta(tft, x, yRsiBar, SCREEN_W - 16, 8, rssiPct, _pRssiPct, rc);
        _pRssiPct = rssiPct;
    }

    // ── HEAP: testo + barra delta ─────────────────────────────
    uint32_t heap = ESP.getFreeHeap();
    uint32_t heapMax = 200000;
    int heapPct = (int)constrain((heap * 100) / heapMax, 0, 100);
    uint16_t hc = heapPct > 50 ? C_GREEN : heapPct > 25 ? C_ORANGE
                                                        : C_RED;

    snprintf(b, sizeof(b), "%u B  ", heap);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(C_LABEL, UI_BG);
    tft.setCursor(x + 56, yHp);
    tft.print(b);

    if (heapPct != _pHeapPct)
    {
        uint16_t prevHc = _pHeapPct > 50 ? C_GREEN : _pHeapPct > 25 ? C_ORANGE
                                                                    : C_RED;
        int oldPct = (hc != prevHc) ? -1 : _pHeapPct; // cambia colore → ridisegno completo
        _barDelta(tft, x, yHpBar, SCREEN_W - 16, 8, heapPct, oldPct, hc);
        _pHeapPct = heapPct;
    }

    // ── Stato DATA ────────────────────────────────────────────
    if (valid != _pValid)
    {
        _pValid = valid;
        _row(tft, x, yDat, "DATA  ", valid ? "LIVE " : "STALE",
             valid ? C_GREEN : C_RED);
    }
}

// =============================================================
//  TAB PASS — countdown grande + barra progresso
// =============================================================
struct PassEntry
{
    uint32_t riseTime;
    uint16_t duration;
    uint8_t maxEl;
};

static PassEntry _passes[PASS_MAX];
static int _passCount = 0;
static bool _passValid = false;
static bool _passError = false;
static char _prevCountdown[24] = "";
static bool _passNeedsFullDraw = true;

void ui_passSetData(PassEntry *entries, int count)
{
    _passCount = min(count, (int)PASS_MAX);
    for (int i = 0; i < _passCount; i++)
        _passes[i] = entries[i];
    _passValid = (_passCount > 0);
    _passError = false;
    _passNeedsFullDraw = true;
    _lastTab = 255;
}
void ui_passSetError()
{
    _passError = true;
    _passValid = false;
    _passCount = 0;
    _passNeedsFullDraw = true;
    _lastTab = 255;
}
bool ui_passNeedsRefresh() { return !_passValid && !_passError; }

void ui_passInvalidate()
{
    _passNeedsFullDraw = true;
    _prevCountdown[0] = '\0';
}

// ── FIX: inline evita ridefinizione con main.cpp ──────────────
inline uint32_t ui_passGetNextRise()
{
    return (_passValid && _passCount > 0) ? _passes[0].riseTime : 0;
}

static void _fmtTime(uint32_t ts, char *buf, size_t sz)
{
    uint32_t t = ts % 86400UL;
    snprintf(buf, sz, "%02u:%02u",
             (unsigned)(t / 3600), (unsigned)((t % 3600) / 60));
}

void ui_drawPass(TFT_eSPI &tft, uint32_t unixNow)
{
    char b[40];
    const int cx = SCREEN_W / 2;

    if (!_passValid && !_passError)
    {
        if (_passNeedsFullDraw)
        {
            _passNeedsFullDraw = false;
            tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, UI_BG);
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextColor(C_LABEL, UI_BG);
            tft.setCursor(cx - 48, SCREEN_H / 2 - 8);
            tft.print("Calcolo in corso...");
        }
        return;
    }
    if (_passError)
    {
        if (_passNeedsFullDraw)
        {
            _passNeedsFullDraw = false;
            tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, UI_BG);
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextColor(C_RED, UI_BG);
            tft.setCursor(cx - 56, SCREEN_H / 2 - 8);
            tft.print("Dati non disponibili");
            tft.setTextColor(C_LABEL, UI_BG);
            tft.setCursor(cx - 48, SCREEN_H / 2 + 12);
            tft.print("Tocca per riprovare");
        }
        return;
    }
    if (_passCount == 0)
    {
        if (_passNeedsFullDraw)
        {
            _passNeedsFullDraw = false;
            tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, UI_BG);
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextColor(C_LABEL, UI_BG);
            tft.setCursor(cx - 70, SCREEN_H / 2 - 8);
            tft.print("Nessun passaggio in 24h");
        }
        return;
    }

    PassEntry &p0 = _passes[0];
    bool visible = (unixNow >= p0.riseTime &&
                    unixNow < p0.riseTime + p0.duration);
    uint32_t diff = (p0.riseTime > unixNow) ? (p0.riseTime - unixNow) : 0;
    uint16_t mainCol = (visible || diff < 600) ? C_GREEN
                       : diff < 3600           ? C_ORANGE
                                               : C_ACCENT;

    if (_passNeedsFullDraw)
    {
        _passNeedsFullDraw = false;
        _prevCountdown[0] = '\0';
        tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, UI_BG);

        int y = BODY_Y + 6;
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, UI_BG);
        const char *lbl = visible ? "ISS SOPRA DI TE ORA" : "PROSSIMO PASSAGGIO";
        tft.setCursor(cx - ((int)strlen(lbl) * 7) / 2, y);
        tft.print(lbl);
        y += 18;
        _box(tft, 8, y, SCREEN_W - 16, 34, mainCol, C_BOXBG);
        y += 40;

        const uint32_t WINDOW = 21600UL;
        int pct = visible           ? 100
                  : (diff < WINDOW) ? (int)(100 - diff * 100 / WINDOW)
                                    : 0;
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(8, y);
        tft.print("T-");
        _bar(tft, 24, y, SCREEN_W - 32, 7, pct, mainCol);
        tft.setCursor(SCREEN_W - 22, y);
        tft.print("T0");
        y += 16;
        tft.drawFastHLine(8, y, SCREEN_W - 16, C_SEP2);
        y += 8;

        char timeStr[8];
        _fmtTime(p0.riseTime, timeStr, sizeof(timeStr));
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(8, y);
        tft.print("Alle");
        tft.setTextColor(C_WHITE, UI_BG);
        snprintf(b, sizeof(b), "%s UTC", timeStr);
        tft.setCursor(52, y);
        tft.print(b);
        y += 18;

        uint16_t dm = p0.duration / 60, ds = p0.duration % 60;
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(8, y);
        tft.print("Durata");
        tft.setTextColor(C_WHITE, UI_BG);
        if (dm > 0)
            snprintf(b, sizeof(b), "~%um %us", dm, ds);
        else
            snprintf(b, sizeof(b), "~%us", p0.duration);
        tft.setCursor(62, y);
        tft.print(b);
        y += 18;

        uint16_t elCol = p0.maxEl >= 40 ? C_GREEN : p0.maxEl >= 20 ? C_ORANGE
                                                                   : C_LABEL;
        const char *elQ = p0.maxEl >= 40 ? "ottima" : p0.maxEl >= 20 ? "buona"
                                                                     : "bassa";
        tft.setTextColor(C_LABEL, UI_BG);
        tft.setCursor(8, y);
        tft.print("Max el.");
        tft.setTextColor(elCol, UI_BG);
        snprintf(b, sizeof(b), "%d\xF7  (%s)", p0.maxEl, elQ);
        tft.setCursor(62, y);
        tft.print(b);
        y += 20;

        if (_passCount > 1 && y < SCREEN_H - 24)
        {
            tft.drawFastHLine(8, y, SCREEN_W - 16, C_SEP);
            y += 6;
            tft.setTextFont(1);
            tft.setTextSize(1);
            tft.setTextColor(C_LABEL, UI_BG);
            tft.setCursor(8, y);
            tft.print("PROSSIMI PASSAGGI:");
            y += 12;
            for (int i = 1; i < _passCount && y < SCREEN_H - 8; i++)
            {
                char t[8];
                _fmtTime(_passes[i].riseTime, t, sizeof(t));
                uint32_t d2 = (_passes[i].riseTime > unixNow)
                                  ? (_passes[i].riseTime - unixNow)
                                  : 0;
                if (d2 < 3600)
                    snprintf(b, sizeof(b), "  %s UTC   tra %um", t, d2 / 60);
                else
                    snprintf(b, sizeof(b), "  %s UTC   tra %uh %02um",
                             t, d2 / 3600, (d2 % 3600) / 60);
                tft.setTextColor(i == 1 ? C_WHITE : C_LABEL, UI_BG);
                tft.setCursor(8, y);
                tft.print(b);
                y += 12;
            }
        }
    }

    // ── Aggiorna solo il countdown ────────────────────────────
    char countdown[24];
    if (visible)
    {
        uint32_t rem = (p0.riseTime + p0.duration) - unixNow;
        snprintf(countdown, sizeof(countdown), "VISIBILE  %um %02us", rem / 60, rem % 60);
    }
    else if (diff < 3600)
    {
        snprintf(countdown, sizeof(countdown), "tra  %um %02us", diff / 60, diff % 60);
    }
    else
    {
        snprintf(countdown, sizeof(countdown), "tra  %uh %02um", diff / 3600, (diff % 3600) / 60);
    }
    if (strcmp(countdown, _prevCountdown) != 0)
    {
        strncpy(_prevCountdown, countdown, sizeof(_prevCountdown));
        int by = BODY_Y + 24;
        tft.fillRect(9, by + 1, SCREEN_W - 18, 32, C_BOXBG);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(mainCol, C_BOXBG);
        int tw2 = (int)strlen(countdown) * 14;
        tft.setCursor(cx - tw2 / 2, by + 4);
        tft.print(countdown);
    }
}