#pragma once
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include "config.h"

// =============================================================
//  OrbitalEye CYD — calibration.h
//  Calibrazione touch a 2 punti, salvata su NVS ESP32.
//  Chiamare calLoad() nel setup per caricare valori salvati.
//  Tenere premuto >3s al boot per ri-calibrare.
// =============================================================

struct TouchCal {
    int16_t xMin = 200;
    int16_t xMax = 3800;
    int16_t yMin = 200;
    int16_t yMax = 3800;
    bool    valid = false;
};

static TouchCal touchCal;

// Salva calibrazione su NVS
static void calSave() {
    Preferences prefs;
    prefs.begin("touchcal", false);
    prefs.putShort("xMin", touchCal.xMin);
    prefs.putShort("xMax", touchCal.xMax);
    prefs.putShort("yMin", touchCal.yMin);
    prefs.putShort("yMax", touchCal.yMax);
    prefs.putBool("valid", true);
    prefs.end();
    Serial.printf("[Cal] salvata: x=%d..%d y=%d..%d\n",
                  touchCal.xMin, touchCal.xMax,
                  touchCal.yMin, touchCal.yMax);
}

// Carica calibrazione da NVS
static bool calLoad() {
    Preferences prefs;
    prefs.begin("touchcal", true);
    bool v = prefs.getBool("valid", false);
    if (v) {
        touchCal.xMin  = prefs.getShort("xMin", 200);
        touchCal.xMax  = prefs.getShort("xMax", 3800);
        touchCal.yMin  = prefs.getShort("yMin", 200);
        touchCal.yMax  = prefs.getShort("yMax", 3800);
        touchCal.valid = true;
        Serial.printf("[Cal] caricata: x=%d..%d y=%d..%d\n",
                      touchCal.xMin, touchCal.xMax,
                      touchCal.yMin, touchCal.yMax);
    } else {
        Serial.println("[Cal] nessuna calibrazione salvata, uso default");
    }
    prefs.end();
    return v;
}

// Mappa raw XPT2046 → coordinate schermo usando la calibrazione corrente
static void calMap(int16_t rawX, int16_t rawY,
                   int16_t& screenX, int16_t& screenY) {
    screenX = map(rawX, touchCal.xMin, touchCal.xMax, 0, SCREEN_W);
    screenY = map(rawY, touchCal.yMin, touchCal.yMax, 0, SCREEN_H);
    screenX = constrain(screenX, 0, SCREEN_W  - 1);
    screenY = constrain(screenY, 0, SCREEN_H - 1);
}

// Disegna mirino di calibrazione
static void calDrawTarget(TFT_eSPI& tft, int x, int y, uint16_t color) {
    tft.fillCircle(x, y, 8, color);
    tft.fillCircle(x, y, 4, TFT_BLACK);
    tft.drawFastHLine(x - 16, y, 32, color);
    tft.drawFastVLine(x, y - 16, 32, color);
}

// Attende un tap e restituisce le coordinate raw
static void calWaitTap(XPT2046_Touchscreen& touch,
                       int16_t& rawX, int16_t& rawY) {
    // Aspetta release prima
    while (touch.touched()) delay(10);
    delay(200);
    // Aspetta press
    while (!touch.touched()) delay(20);
    delay(50); // debounce
    TS_Point p = touch.getPoint();
    rawX = p.x;
    rawY = p.y;
    Serial.printf("[Cal] tap raw: %d, %d\n", rawX, rawY);
    // Aspetta release
    while (touch.touched()) delay(10);
    delay(200);
}

// Procedura di calibrazione completa
// Mostra due mirini (TL e BR), misura i raw values, salva
void runCalibration(TFT_eSPI& tft, XPT2046_Touchscreen& touch) {
    const int MARGIN = 20;

    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("TOUCH CALIBRATION", SCREEN_W/2, 30);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Tap the circle", SCREEN_W/2, 55);

    // --- Punto 1: Top-Left ---
    int p1x = MARGIN, p1y = MARGIN;
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("CALIBRATION  1/2", SCREEN_W/2, SCREEN_H/2);
    calDrawTarget(tft, p1x, p1y, TFT_YELLOW);

    int16_t r1x, r1y;
    calWaitTap(touch, r1x, r1y);
    calDrawTarget(tft, p1x, p1y, TFT_GREEN);

    // --- Punto 2: Bottom-Right ---
    int p2x = SCREEN_W - MARGIN, p2y = SCREEN_H - MARGIN;
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("CALIBRATION  2/2", SCREEN_W/2, SCREEN_H/2);
    calDrawTarget(tft, p2x, p2y, TFT_YELLOW);

    int16_t r2x, r2y;
    calWaitTap(touch, r2x, r2y);
    calDrawTarget(tft, p2x, p2y, TFT_GREEN);

    // --- Calcola calibrazione ---
    touchCal.xMin  = min(r1x, r2x);
    touchCal.xMax  = max(r1x, r2x);
    touchCal.yMin  = min(r1y, r2y);
    touchCal.yMax  = max(r1y, r2y);
    touchCal.valid = true;

    calSave();

    // --- Conferma ---
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("CALIBRATION OK!", SCREEN_W/2, SCREEN_H/2 - 16);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char buf[32];
    snprintf(buf, sizeof(buf), "x: %d - %d", touchCal.xMin, touchCal.xMax);
    tft.drawString(buf, SCREEN_W/2, SCREEN_H/2 + 6);
    snprintf(buf, sizeof(buf), "y: %d - %d", touchCal.yMin, touchCal.yMax);
    tft.drawString(buf, SCREEN_W/2, SCREEN_H/2 + 24);
    delay(2000);
}