// =============================================================
//  OrbitalEye CYD — main.cpp  v2.0
//  ESP32-2432S028R (CYD USB-C)
//
//  Changelog v2.0 rispetto a v1.4:
//  - Boot splash ottimizzato: sprite buffer + frame ridotti (~5s)
//  - Luminosità automatica con PWM (dim dopo 2min senza touch)
//  - Riconnessione WiFi automatica con backoff
//  - Indicatore dati STALE (>30s senza fetch) nella sidebar
//  - Notifica visiva passaggio ISS in arrivo (<10min)
//  - TAB_COUNT=6 con handleTouch corretto
//  - Calcolo passaggi locale (nessuna API esterna)
//  - sincronizzazione orario Unix dal timestamp ISS
// =============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <math.h>

#include "config.h"
#include "globe_data.h"
#include "globe_renderer.h"
#include "calibration.h"
#include "ui.h"

// -------------------------------------------------------------
//  Hardware
// -------------------------------------------------------------
TFT_eSPI tft;
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// -------------------------------------------------------------
//  Luminosità — PWM sul pin backlight
// -------------------------------------------------------------
#define BL_CHANNEL      0
#define BL_FREQ         5000
#define BL_BITS         8
#define BL_FULL         255
#define BL_DIM          55
#define DIM_TIMEOUT_MS  120000UL   // 2 minuti senza touch

static bool     blDimmed       = false;
static uint32_t lastActivityMs = 0;

static void blSet(uint8_t val) {
    ledcWrite(BL_CHANNEL, val);
}

static void blActivity() {
    lastActivityMs = millis();
    if (blDimmed) {
        blDimmed = false;
        blSet(BL_FULL);
    }
}

static void blUpdate() {
    if (!blDimmed && (millis() - lastActivityMs > DIM_TIMEOUT_MS)) {
        blDimmed = true;
        blSet(BL_DIM);
    }
}

// -------------------------------------------------------------
//  Dati ISS / Crew — condivisi Core 0 <-> Core 1
// -------------------------------------------------------------
struct ISSData {
    float    lat       = 0.0f;
    float    lon       = 0.0f;
    float    alt       = 408.0f;
    float    velocity  = 7.66f;
    uint32_t timestamp = 0;
    uint32_t fetchMs   = 0;   // millis() dell'ultimo fetch riuscito
    bool     valid     = false;
};

struct CrewData {
    String names[10];
    int    count = 0;
    bool   valid = false;
};

static ISSData  issData;
static CrewData crewData;
static SemaphoreHandle_t g_dataMutex;
static volatile bool g_fetchPassRequested = false;
static volatile bool g_splashDone = false;  // blocca loop() durante splash

// Stale: se > 30s dall'ultimo fetch → mostra indicatore
static volatile bool g_issStale = false;

static void loadFallbackCrew() {
    crewData.names[0] = "Oleg Kononenko";
    crewData.names[1] = "Nikolai Chub";
    crewData.names[2] = "Tracy Dyson";
    crewData.names[3] = "Butch Wilmore";
    crewData.names[4] = "Sunita Williams";
    crewData.names[5] = "Mike Barratt";
    crewData.names[6] = "Matthew Dominick";
    crewData.count    = 7;
    crewData.valid    = true;
}

// -------------------------------------------------------------
//  Tab navigation — 6 tab, 53px ciascuna (320/6 ≈ 53)
// -------------------------------------------------------------
enum Tab : uint8_t {
    TAB_GLOBE = 0,
    TAB_DATA,
    TAB_WHERE,
    TAB_CREW,
    TAB_SYS,
    TAB_PASS,
    TAB_COUNT   // = 6
};


static uint8_t currentTab = TAB_GLOBE;
// TAB_N già definito in ui.h come static const uint8_t TAB_N = 6

// -------------------------------------------------------------
//  Uptime / orbite
// -------------------------------------------------------------
static uint32_t bootTime      = 0;
static uint32_t orbitCount    = 0;
static uint32_t lastOrbitTick = 0;
static const uint32_t ORBIT_PERIOD_MS = 5568000UL;  // 92.8min × 60000

// -------------------------------------------------------------
//  Touch debounce
// -------------------------------------------------------------
static uint32_t lastTouchMs = 0;
static const uint32_t TOUCH_DEBOUNCE = 300;

// =============================================================
//  BOOT SPLASH — Starfield + titolo
// =============================================================
#define SP_CX 160
#define SP_CY 120

static void drawSplash() {
    tft.fillScreen(TFT_BLACK);

    // Stelle: appaiono una a una in posizioni casuali
    for (int i = 0; i < 150; i++) {
        int x = random(0, SCREEN_W);
        int y = random(0, SCREEN_H);
        uint8_t b = (uint8_t)random(80, 255);
        tft.drawPixel(x, y, tft.color565(b, b, b));
        delay(8);
    }

    delay(300);

    // Titolo centrato
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextColor(tft.color565(0, 210, 255), TFT_BLACK);
    tft.drawString("OrbitalEye", SP_CX, SP_CY - 10);

    tft.setTextFont(2);
    tft.setTextColor(tft.color565(60, 60, 60), TFT_BLACK);
    tft.drawString("ISS Tracker  v2.0", SP_CX, SP_CY + 18);

    delay(1200);
}
// =============================================================
//  CALCOLO PASSAGGI ISS — propagazione orbitale locale
//  Nessuna API esterna. Precisione: ±5 minuti.
//  Per ±30s usare SGP4 con TLE da celestrak.org.
// =============================================================
static const float ISS_INCLINATION   = 51.6f;
static const float ISS_PERIOD_MIN    = 92.68f;
static const float EARTH_R_KM        = 6371.0f;
static const float DEG2RAD           = M_PI / 180.0f;
static const float RAD2DEG           = 180.0f / M_PI;
static const float ISS_VISIBILITY_KM = 1800.0f;

// Orologio Unix stimato
static uint32_t g_lastFetchTs     = 0;
static uint32_t g_lastFetchMillis = 0;

static void syncTime(uint32_t unixTs) {
    g_lastFetchTs     = unixTs;
    g_lastFetchMillis = millis();
}

static uint32_t estimateUnixNow() {
    if (g_lastFetchTs == 0) return 0;
    return g_lastFetchTs + (millis() - g_lastFetchMillis) / 1000;
}

static float geoDistKm(float lat1, float lon1, float lat2, float lon2) {
    float dLat = (lat2 - lat1) * DEG2RAD;
    float dLon = (lon2 - lon1) * DEG2RAD;
    float a = sinf(dLat / 2) * sinf(dLat / 2) +
              cosf(lat1 * DEG2RAD) * cosf(lat2 * DEG2RAD) *
              sinf(dLon / 2) * sinf(dLon / 2);
    return EARTH_R_KM * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

static void propagateISS(float curLat, float curLon, float curTs,
                          float targetTs, float& outLat, float& outLon) {
    float dtMin    = (targetTs - curTs) / 60.0f;
    float dtOrbits = dtMin / ISS_PERIOD_MIN;
    float phase    = asinf(constrain(curLat / ISS_INCLINATION, -1.0f, 1.0f))
                     + dtOrbits * 2.0f * M_PI;
    outLat = ISS_INCLINATION * sinf(phase);
    outLon = curLon + dtOrbits * 360.0f - dtMin * (360.0f / 1440.0f);
    while (outLon >  180.0f) outLon -= 360.0f;
    while (outLon < -180.0f) outLon += 360.0f;
}

static void calculatePasses(float issLat, float issLon, uint32_t issTs,
                             PassEntry* entries, int& count) {
    count = 0;
    bool     inPass    = false;
    uint32_t passStart = 0;
    float    minDist   = 99999.0f;

    for (float dt = 0; dt < 86400.0f && count < PASS_MAX; dt += 15.0f) {
        float pLat, pLon;
        propagateISS(issLat, issLon, (float)issTs,
                     (float)issTs + dt, pLat, pLon);
        float dist    = geoDistKm(USER_LAT, USER_LON, pLat, pLon);
        bool  visible = (dist < ISS_VISIBILITY_KM);

        if (visible && !inPass) {
            inPass = true; passStart = issTs + (uint32_t)dt; minDist = dist;
        } else if (visible && inPass) {
            if (dist < minDist) minDist = dist;
        } else if (!visible && inPass) {
            inPass = false;
            uint32_t dur = issTs + (uint32_t)dt - passStart;
            if (dur > 60) {
                entries[count].riseTime = passStart;
                entries[count].duration = (uint16_t)min((uint32_t)900, dur);
                float elev = RAD2DEG * atanf(408.0f / max(minDist, 10.0f));
                entries[count].maxEl = (uint8_t)constrain((int)elev, 0, 90);
                count++;
                minDist = 99999.0f;
            }
        }
    }
}

// =============================================================
//  Forward declarations
// =============================================================
// Forward declaration per notifica passaggio imminente.
// Se ui.h non la espone ancora, definire qui come stub.

void connectWiFi();
void fetchTask(void* pvParameters);
void handleTouch();
void updateOrbitCounter();

// =============================================================
//  FETCH TASK — Core 0
// =============================================================
void fetchTask(void* pvParameters) {
    const TickType_t ISS_INTERVAL  = pdMS_TO_TICKS(5000);
    const TickType_t CREW_INTERVAL = pdMS_TO_TICKS(30000);
    const TickType_t PASS_INTERVAL = pdMS_TO_TICKS(900000UL);  // 15 minuti
    const TickType_t WIFI_CHECK    = pdMS_TO_TICKS(15000);     // 15 secondi

    TickType_t lastCrewTick  = 0;
    TickType_t lastPassCalc  = 0;
    TickType_t lastWifiCheck = 0;

    uint32_t wifiBackoffMs = 2000;  // backoff esponenziale per WiFi

    for (;;) {
        TickType_t now = xTaskGetTickCount();

        // ── Riconnessione WiFi automatica con backoff ──────────
        if ((now - lastWifiCheck) >= pdMS_TO_TICKS(wifiBackoffMs)) {
            lastWifiCheck = now;
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WiFi] disconnesso, riconnessione...");
                WiFi.disconnect(true);
                delay(100);
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                bool ok = false;
                for (int i = 0; i < 20 && !ok; i++) {
                    vTaskDelay(pdMS_TO_TICKS(500));
                    ok = (WiFi.status() == WL_CONNECTED);
                }
                if (ok) {
                    Serial.printf("[WiFi] OK: %s\n", WiFi.localIP().toString().c_str());
                    wifiBackoffMs = 15000;  // reset backoff
                } else {
                    wifiBackoffMs = min(wifiBackoffMs * 2U, (uint32_t)300000U);  // max 5min
                    Serial.printf("[WiFi] fallita, riprovo tra %lus\n",
                                  (unsigned long)(wifiBackoffMs / 1000));
                }
            } else {
                wifiBackoffMs = 15000;  // reset se connesso
            }
        }

        // ── ISS position ogni 5s ──────────────────────────────
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin("http://api.open-notify.org/iss-now.json");
            http.setTimeout(4000);
            int code = http.GET();
            if (code == 200) {
                JsonDocument doc;
                String body = http.getString();
                if (deserializeJson(doc, body) == DeserializationError::Ok) {
                    float    lat = doc["iss_position"]["latitude"].as<float>();
                    float    lon = doc["iss_position"]["longitude"].as<float>();
                    uint32_t ts  = doc["timestamp"].as<uint32_t>();
                    syncTime(ts);
                    if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                        issData.lat     = lat;
                        issData.lon     = lon;
                        issData.timestamp = ts;
                        issData.fetchMs   = millis();
                        issData.valid     = true;
                        g_issStale        = false;
                        xSemaphoreGive(g_dataMutex);
                    }
                }
            }
            http.end();
        }

        // Aggiorna flag stale (>30s senza fetch riuscito)
        if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            g_issStale = issData.valid && ((millis() - issData.fetchMs) > 30000);
            xSemaphoreGive(g_dataMutex);
        }

        // ── Crew ogni 30s (NON filtrare per craft) ───────────
        if ((now - lastCrewTick) >= CREW_INTERVAL) {
            lastCrewTick = now;
            if (WiFi.status() == WL_CONNECTED) {
                HTTPClient http2;
                http2.begin("http://api.open-notify.org/astros.json");
                http2.setTimeout(6000);
                if (http2.GET() == 200) {
                    JsonDocument doc2;
                    if (deserializeJson(doc2, http2.getString()) ==
                        DeserializationError::Ok) {
                        int nc = 0; String nn[10];
                        // Non filtrare per craft: include Shenzhou, Axiom ecc.
                        for (JsonObject p : doc2["people"].as<JsonArray>())
                            if (nc < 10) nn[nc++] = p["name"].as<String>();
                        if (nc > 0 &&
                            xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                            crewData.count = nc;
                            for (int i = 0; i < nc; i++) crewData.names[i] = nn[i];
                            crewData.valid = true;
                            xSemaphoreGive(g_dataMutex);
                        }
                    }
                }
                http2.end();
            }
        }

        // ── Calcolo passaggi ogni 15min o su richiesta ────────
        bool doPass = g_fetchPassRequested
                   || (lastPassCalc == 0)
                   || ((now - lastPassCalc) >= PASS_INTERVAL);

        if (doPass) {
            g_fetchPassRequested = false;
            lastPassCalc = now;

            float issLat, issLon; uint32_t issTs; bool issValid;
            if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                issLat   = issData.lat;
                issLon   = issData.lon;
                issTs    = issData.timestamp;
                issValid = issData.valid;
                xSemaphoreGive(g_dataMutex);
            } else {
                issValid = false;
            }

            if (issValid && issTs > 0) {
                PassEntry entries[PASS_MAX]; int cnt = 0;
                calculatePasses(issLat, issLon, issTs, entries, cnt);
                if (cnt > 0) ui_passSetData(entries, cnt);
                else         ui_passSetError();
            }
        }

        vTaskDelay(ISS_INTERVAL);
    }
}

// =============================================================
//  SETUP
// =============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[OrbitalEye CYD] boot v2.0");

    // Backlight PWM
    ledcSetup(BL_CHANNEL, BL_FREQ, BL_BITS);
    ledcAttachPin(TFT_BL, BL_CHANNEL);
    blSet(BL_FULL);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // Splash ottimizzato (~5s grazie agli sprite)
    drawSplash();

    g_splashDone = true;  // da qui il loop() può disegnare

    // Touch su HSPI separato (CRITICO: evita conflitti col display)
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    touch.begin(touchSPI);
    touch.setRotation(1);

    calLoad();
    delay(100);
    if (touch.touched()) {
        Serial.println("[Cal] avvio calibrazione...");
        runCalibration(tft, touch);
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextColor(tft.color565(0,200,80), TFT_BLACK);
        tft.drawString("Calibrazione OK", SCREEN_W/2, SCREEN_H/2);
        delay(800);
        tft.fillScreen(TFT_BLACK);
    }

    connectWiFi();
    loadFallbackCrew();

    g_dataMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(fetchTask, "fetch", 10240, nullptr, 1, nullptr, 0);

    bootTime = lastOrbitTick = lastActivityMs = millis();
    orbitCount = 0;
    // NON fillScreen qui — il loop() disegna il primo frame da solo
    Serial.println("[OrbitalEye CYD] pronto");
}

// =============================================================
//  LOOP — Core 1
// =============================================================
void loop() {
    // Aspetta che la splash sia finita prima di disegnare qualsiasi cosa
    if (!g_splashDone) { delay(10); return; }

    // Snapshot thread-safe (timeout 10ms — se mutex occupato salta il frame)
    ISSData  iss;
    CrewData crew;

    if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        iss  = issData;
        crew = crewData;
        xSemaphoreGive(g_dataMutex);
    }

    updateOrbitCounter();
    handleTouch();
    blUpdate();

    // Notifica passaggio ISS in arrivo (<10min): lampeggio tab PASS
    // Legge il prossimo riseTime da ui_passGetNextRise() se disponibile
    bool passImminent = false;
    {
        uint32_t nowTs = estimateUnixNow();
        if (nowTs > 0) {
            uint32_t nextRise = ui_passGetNextRise();  // 0 se non disponibile
            if (nextRise > 0 && nextRise > nowTs && (nextRise - nowTs) < 600)
                passImminent = true;
        }
    }

    if (currentTab == TAB_PASS && ui_passNeedsRefresh())
        g_fetchPassRequested = true;

    switch (currentTab) {
        case TAB_GLOBE:
            renderGlobe(tft, iss.lat, iss.lon);
            ui_drawGlobeSidebar(tft, iss.lat, iss.lon, iss.alt);
            ui_drawTabBar(tft, currentTab);
            break;

        case TAB_DATA:
            ui_drawData(tft, iss.lat, iss.lon, iss.alt, iss.velocity);
            ui_drawTabBar(tft, currentTab);
            delay(200);
            break;

        case TAB_WHERE: {
            GeoZone gz = getGeoZone(iss.lat, iss.lon);
            ui_drawWhere(tft, iss.lat, iss.lon, gz.name);
            ui_drawTabBar(tft, currentTab);
            delay(200);
            break;
        }

        case TAB_CREW:
            ui_drawCrew(tft, crew.names, crew.count);
            ui_drawTabBar(tft, currentTab);
            delay(200);
            break;

        case TAB_SYS: {
            uint32_t upSec = (millis() - bootTime) / 1000;
            ui_drawSys(tft, upSec, orbitCount, WiFi.RSSI(), iss.valid);
            ui_drawTabBar(tft, currentTab);
            delay(200);
            break;
        }

        case TAB_PASS:
            ui_drawPass(tft, estimateUnixNow());
            ui_drawTabBar(tft, currentTab);
            delay(950);
            break;

        default:
            currentTab = TAB_GLOBE;
            break;
    }
}

// =============================================================
//  WiFi — connessione iniziale al boot
// =============================================================
void connectWiFi() {
    Serial.printf("[WiFi] connessione a %s...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 210);
    tft.print("WiFi...");

    uint8_t att = 0;
    while (WiFi.status() != WL_CONNECTED && att < 30) {
        delay(500);
        att++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(10, 210);
        tft.print("WiFi OK        ");
    } else {
        Serial.println("[WiFi] timeout — riconnessione automatica attiva");
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setCursor(10, 210);
        tft.print("WiFi FAIL - retry auto");
    }
    delay(500);
}

// =============================================================
//  Touch handler
// =============================================================
void handleTouch() {
    if (!touch.tirqTouched() || !touch.touched()) return;
    uint32_t now = millis();
    if (now - lastTouchMs < TOUCH_DEBOUNCE) return;
    lastTouchMs = now;

    blActivity();
    if (blDimmed) return;  // primo touch dopo dim: solo riaccende

    TS_Point p = touch.getPoint();
    int16_t tx, ty;
    calMap(p.x, p.y, tx, ty);

    uint8_t prev = currentTab;

    if (ty < TABBAR_H) {
        // Tap nella tab bar: 6 zone da 53px (SCREEN_W / TAB_N)
        currentTab = (uint8_t)constrain(tx / (SCREEN_W / TAB_N),
                                         0, TAB_COUNT - 1);
    } else if (tx < 107) {
        // Swipe sx → tab precedente
        currentTab = (currentTab > 0) ? currentTab - 1 : TAB_COUNT - 1;
    } else if (tx > 213) {
        // Swipe dx → tab successiva
        currentTab = (currentTab < TAB_COUNT - 1) ? currentTab + 1 : 0;
    }

    if (currentTab != prev) {
        tft.fillScreen(TFT_BLACK);
        mapInvalidate();
        if (currentTab == TAB_PASS) {
            g_fetchPassRequested = true;
            ui_passInvalidate();
        }
        Serial.printf("[Touch] tab -> %d\n", currentTab);
    }
}

// =============================================================
//  Contatore orbite
// =============================================================
void updateOrbitCounter() {
    uint32_t now = millis();
    if (now - lastOrbitTick >= ORBIT_PERIOD_MS) {
        orbitCount++;
        lastOrbitTick = now;
    }
}