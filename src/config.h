#pragma once
#include <stdint.h>

// =============================================================
//  OrbitalEye CYD — config.h
//  Hardware: ESP32-2432S028R (CYD USB-C)
//  Display:  ILI9341 320x240 su VSPI
//  Touch:    XPT2046 su HSPI separato
// =============================================================

// ── WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"

// ── Posizione utente (per calcolo passaggi ISS locale) ────────
#define USER_LAT        41.9f    // latitudine  (es. Roma)
#define USER_LON        12.5f    // longitudine (es. Roma)
#define USER_ALT        0        // altitudine in metri

// ── Pin Touch XPT2046 (HSPI dedicato — CRITICO per CYD) ──────
#define TOUCH_CLK       25
#define TOUCH_MISO      39
#define TOUCH_MOSI      32
#define TOUCH_CS        33
#define TOUCH_IRQ       36

// ── Pin Display ───────────────────────────────────────────────
#define TFT_BL          21       // backlight

// ── Dimensioni schermo ────────────────────────────────────────
#define SCREEN_W        320
#define SCREEN_H        240

// ── Layout UI ─────────────────────────────────────────────────
#define TABBAR_H        28       // altezza barra tab
#define SIDEBAR_X       244      // X di partenza sidebar nel tab GLOBE

// ── Globe renderer ────────────────────────────────────────────
#define GLOBE_R         110      // raggio del globo

// ── Tab PASS ──────────────────────────────────────────────────
#define PASS_MAX        5        // numero massimo di passaggi memorizzati

// ── Timing aggiornamenti (millisecondi) ───────────────────────
#define ISS_UPDATE_MS       5000     // posizione ISS
#define CREW_UPDATE_MS     30000     // equipaggio
#define PASS_UPDATE_MS    900000     // calcolo passaggi (15 min)
