/*
  OBDuke firmware v1.2
  Gen1 KTM 390 Duke diagnostic tool - ESP32-WROOM-32E + SN65HVD230

  Links (all optional, set in Settings):
    Wi-Fi hotspot  -> web page at http://1.1.1.1 (or http://obduke.local on home Wi-Fi)
    Wi-Fi hotspot  -> OBDuke Android app (same /api as the web page)
    BT Classic     -> ELM327 emulator for Torque Pro or the Teltonika tracker, or the
                      OBDuke Android app (lines starting with '!' get JSON replies). One at a time.
    USB serial     -> same text commands as the app (115200 baud)

  Arduino IDE: Board "ESP32 Dev Module", Partition Scheme "Huge APP (3MB No OTA/1MB SPIFFS)".
  No extra libraries needed. Keep obduke.ino, obduke_fw.cpp and web_ui.h together.
  (Code is in a .cpp so Arduino's auto-prototype step can't scramble the type order.)

  Wiring:
    SN65HVD230 CTX -> GPIO21, CRX -> GPIO22 (changeable in Settings), 3V3, GND
    CANH -> OBD2 pin 6, CANL -> OBD2 pin 14, ground shared with the bike
    Optional battery sense: 12V -> 47k -> GPIO34 -> 10k -> GND (set vbat_pin 34, vbat_mult 5.7)
*/

#include <Arduino.h>
#include <stdarg.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "esp_arduino_version.h"
#if __has_include("esp_mac.h")
#include "esp_mac.h"
#endif
#include "driver/twai.h"
#include "BluetoothSerial.h"
#include "esp_bt.h"
#include "web_ui.h"

#define FW_VERSION "1.2.1"
#define OBD_FUNC_ID 0x7DF


// =====================================================================
//  Settings (stored in NVS, edited from the web page, app or serial)
// =====================================================================
enum SType : uint8_t { S_STR, S_PASS, S_INT, S_BOOL, S_FLOAT, S_SEL };
struct SDef {
  const char *key, *label, *group;
  SType type;
  const char *def;
  bool reboot;
  const char *opts;       // S_SEL: "0:Label|1:Label"
  float mn, mx;           // numeric range, or max length for strings
};

enum {
  K_WIFI_MODE, K_AP_SSID, K_AP_PASS, K_AP_IP, K_AP_CH, K_AP_HIDDEN, K_STA_SSID, K_STA_PASS, K_HOSTNAME,
  K_WEB_USER, K_WEB_PASS, K_CAPTIVE,
  K_BT_NAME,
  K_TRK_ON, K_TRK_PIN, K_TRK_VIRT,
  K_CAN_RATE, K_CAN_TX, K_CAN_RX, K_RPM_DIV, K_ALLOW_RAW,
  K_UNITS, K_LIVE_MS, K_TEMP_WARN, K_RPM_WARN,
  K_GEARS,
  K_SIG1, K_SIG2, K_SIG3, K_SIG4,
  K_VBAT_PIN, K_VBAT_MULT, K_VBAT_LOW,
  K_LOG_MS, K_LOG_AUTO, K_LED_PIN,
  K_COUNT
};

#define SIG_HELP "SOURCE:byte:length:b/l:scale:offset:name:unit"

static const SDef SDEFS[K_COUNT] = {
  {"wifi_mode", "Wi-Fi mode", "Wi-Fi", S_SEL, "0", true, "0:Hotspot only|1:Hotspot + home Wi-Fi|2:Wi-Fi off", 0, 2},
  {"ap_ssid",   "Hotspot name", "Wi-Fi", S_STR, "OBDuke", true, "", 1, 32},
  {"ap_pass",   "Hotspot password (8+ chars, blank = open)", "Wi-Fi", S_PASS, "obduke1234", true, "", 0, 63},
  {"ap_ip",     "Hotspot address (web page IP)", "Wi-Fi", S_STR, "1.1.1.1", true, "", 7, 15},
  {"ap_ch",     "Hotspot channel", "Wi-Fi", S_INT, "6", true, "", 1, 13},
  {"ap_hidden", "Hide hotspot name", "Wi-Fi", S_BOOL, "0", true, "", 0, 1},
  {"sta_ssid",  "Home Wi-Fi name", "Wi-Fi", S_STR, "", true, "", 0, 32},
  {"sta_pass",  "Home Wi-Fi password", "Wi-Fi", S_PASS, "", true, "", 0, 63},
  {"hostname",  "Network name (.local)", "Wi-Fi", S_STR, "obduke", true, "", 1, 24},

  {"web_user",  "Web login user (blank = no login)", "Web page", S_STR, "admin", false, "", 0, 24},
  {"web_pass",  "Web login password (blank = no login)", "Web page", S_PASS, "", false, "", 0, 32},
  {"captive",   "Open page automatically on join", "Web page", S_BOOL, "1", true, "", 0, 1},

  {"bt_name",   "Bluetooth name (Torque / tracker see this)", "Bluetooth ELM327", S_STR, "OBDuke", true, "", 1, 24},

  {"trk_on",    "Bluetooth ELM327 link (Torque Pro / tracker)", "Bluetooth ELM327", S_BOOL, "1", true, "", 0, 1},
  {"trk_pin",   "Bluetooth pairing PIN", "Bluetooth ELM327", S_STR, "1234", true, "", 4, 16},
  {"trk_virt",  "Virtual PIDs (RPM, battery, gear, custom signals)", "Bluetooth ELM327", S_BOOL, "1", false, "", 0, 1},

  {"can_rate",  "CAN speed", "CAN / ECU", S_SEL, "0", true, "0:Auto|1:500 kbps|2:250 kbps", 0, 2},
  {"can_tx",    "CAN TX pin (to CTX)", "CAN / ECU", S_INT, "21", true, "", 0, 33},
  {"can_rx",    "CAN RX pin (to CRX)", "CAN / ECU", S_INT, "22", true, "", 0, 39},
  {"rpm_div",   "KTM RPM divisor", "CAN / ECU", S_FLOAT, "4", false, "", 1, 16},
  {"allow_raw", "Allow raw ECU commands (advanced)", "CAN / ECU", S_BOOL, "0", false, "", 0, 1},

  {"units",     "Units", "Display", S_SEL, "0", false, "0:Metric|1:Imperial", 0, 1},
  {"live_ms",   "Live data refresh (ms)", "Display", S_INT, "150", false, "", 100, 5000},
  {"temp_warn", "Coolant warning (C)", "Display", S_INT, "105", false, "", 60, 130},
  {"rpm_warn",  "Shift / RPM warning", "Display", S_INT, "9500", false, "", 1000, 12000},

  // rpm per km/h in each gear. Defaults from Gen1 390 Duke gearing (30:80 primary, 15:45 final,
  // 150/60-17 rear). Use Tools > Gear calibration to learn your bike's real values.
  {"gears",     "Gear ratios, rpm per km/h (1st..6th)", "Gear indicator", S_STR, "175,131,99,82,73,67", false, "", 0, 63},

  {"sig1",      "Signal 1 (" SIG_HELP ")", "Custom signals", S_STR, "", false, "", 0, 63},
  {"sig2",      "Signal 2", "Custom signals", S_STR, "", false, "", 0, 63},
  {"sig3",      "Signal 3", "Custom signals", S_STR, "", false, "", 0, 63},
  {"sig4",      "Signal 4", "Custom signals", S_STR, "", false, "", 0, 63},

  {"vbat_pin",  "Battery sense pin (0 = off, use 32-39)", "Battery", S_INT, "0", true, "", 0, 39},
  {"vbat_mult", "Battery divider ratio", "Battery", S_FLOAT, "5.7", false, "", 1, 20},
  {"vbat_low",  "Low battery warning (V)", "Battery", S_FLOAT, "11.8", false, "", 9, 14},

  {"log_ms",    "Log interval (ms)", "Logging", S_INT, "300", false, "", 200, 60000},
  {"log_auto",  "Start logging on power-up", "Logging", S_BOOL, "0", false, "", 0, 1},
  {"led_pin",   "Status LED pin (0 = off)", "System", S_INT, "2", true, "", 0, 33},
};

static String sv[K_COUNT];
static Preferences prefs;

static int    sInt(int k)   { return sv[k].toInt(); }
static float  sFlt(int k)   { return sv[k].toFloat(); }
static bool   sBool(int k)  { return sv[k].toInt() != 0; }
static const String &sStr(int k) { return sv[k]; }

static void loadSettings() {
  prefs.begin("obduke", true);
  for (int i = 0; i < K_COUNT; i++) sv[i] = prefs.getString(SDEFS[i].key, SDEFS[i].def);
  prefs.end();
}

static void saveSetting(int k) {
  prefs.begin("obduke", false);
  prefs.putString(SDEFS[k].key, sv[k]);
  prefs.end();
}

// =====================================================================
//  Globals
// =====================================================================
BluetoothSerial SerialBT;
WebServer web(80);
DNSServer dns;

static bool serialLive = false;
static bool webStarted = false, dnsStarted = false, trkStarted = false;
static uint32_t webLiveUntil = 0, rebootAt = 0;

static uint32_t ecuRespId = 0, lastRespId = 0;
static bool ecuFound = false;
static uint32_t lastPing = 0;
static int canRateIdx = 0;
static uint8_t suppBits[24];   // supported mode 01 PIDs 01..C0
static bool ktmRpmOk = false;

// =====================================================================
//  JSON helpers
// =====================================================================
static String jEsc(const String &s) {
  String o;
  o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '"':  o += "\\\""; break;
      case '\\': o += "\\\\"; break;
      case '\n': o += "\\n";  break;
      case '\r': o += "\\r";  break;
      case '\t': o += "\\t";  break;
      default:
        if ((uint8_t)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
        else o += c;
    }
  }
  return o;
}
static String q(const String &s) { return "\"" + jEsc(s) + "\""; }
static String errJson(const char *t, const String &msg) {
  return String("{\"t\":\"") + t + "\",\"ok\":false,\"msg\":" + q(msg) + "}";
}
static String toHex(const uint8_t *d, int n, bool sp = true) {
  String o;
  char b[4];
  for (int i = 0; i < n; i++) {
    snprintf(b, sizeof(b), sp && i ? " %02X" : "%02X", d[i]);
    o += b;
  }
  return o;
}
static String toAscii(const uint8_t *d, int n) {
  String o;
  for (int i = 0; i < n; i++) o += (d[i] >= 0x20 && d[i] < 0x7F) ? (char)d[i] : '.';
  return o;
}
static String fmtF(float v, int dp) { return String(v, (unsigned int)dp); }

// =====================================================================
//  Bus monitor table
// =====================================================================
// Every ID seen on the bus, with which bytes changed since the last read (chg) and
// since "mark" (mk), plus each byte's min/max since mark. Used by the sniffer and monitor.
struct IdEntry {
  uint32_t id; bool ext; uint8_t dlc; uint8_t data[8], lo[8], hi[8];
  uint8_t chg, mk; uint32_t count, first, last;
};
#define MAX_IDS 64
static IdEntry ids[MAX_IDS];
static int idCount = 0;
static bool monitorOn = false;
static uint32_t sniffPolled = 0, sniffStart = 0;

// ---------- custom signals: decode a value out of a broadcast frame or a KTM local ID ----------
struct Sig {
  bool on = false, lid = false, little = false, sgn = false;
  uint32_t id = 0; uint8_t lidNum = 0, byte = 0, len = 1;
  float scale = 1, off = 0;
  String name, unit;
  float val = 0; uint32_t at = 0;
};
static Sig sigs[4];

static bool sigExtract(const Sig &s, const uint8_t *d, int n, float &out) {
  if (s.byte + s.len > n) return false;
  uint32_t v = 0;
  for (int i = 0; i < s.len; i++) {
    uint8_t b = d[s.byte + i];
    if (s.little) v |= (uint32_t)b << (8 * i);
    else v = (v << 8) | b;
  }
  int32_t sv32 = (int32_t)v;
  if (s.sgn && s.len < 4 && (v & (1UL << (s.len * 8 - 1)))) sv32 = (int32_t)(v | (0xFFFFFFFFUL << (s.len * 8)));
  out = (s.sgn ? (float)sv32 : (float)v) * s.scale + s.off;
  return true;
}

// "SOURCE:byte:length:b/l:scale:offset:name:unit"
//   SOURCE = broadcast CAN ID in hex (e.g. 120), or a KTM local ID read (e.g. 2122)
//   b/l = big / little endian, B/L = signed. Returns "" if OK, else the problem.
static String sigParse(const String &txt, Sig &s) {
  s = Sig();
  String t = txt; t.trim();
  if (!t.length()) return "";
  String f[8];
  int nf = 0, start = 0;
  for (int i = 0; i <= (int)t.length() && nf < 8; i++) {
    if (i == (int)t.length() || t[i] == ':') { f[nf++] = t.substring(start, i); start = i + 1; }
  }
  if (nf < 6) return "Format: " SIG_HELP ", e.g. 120:2:2:b:0.01:0:Front wheel:km/h";
  for (int i = 0; i < nf; i++) f[i].trim();
  String src = f[0]; src.toUpperCase();
  if (src.startsWith("0X")) src = src.substring(2);
  for (size_t i = 0; i < src.length(); i++) if (!isxdigit(src[i])) return "Source must be hex, e.g. 120 or 2122";
  if (src.length() == 4 && src.startsWith("21")) { s.lid = true; s.lidNum = strtoul(src.substring(2).c_str(), nullptr, 16); }
  else if (src.length() >= 1 && src.length() <= 8) s.id = strtoul(src.c_str(), nullptr, 16);
  else return "Source must be a CAN ID (e.g. 120) or a KTM local ID (e.g. 2122)";
  s.byte = f[1].toInt();
  s.len = f[2].toInt();
  if (s.len < 1 || s.len > 4) return "Length must be 1-4 bytes";
  if (s.byte > 60 || (!s.lid && s.byte + s.len > 8)) return "Byte position is past the end of the frame";
  char e = f[3].length() ? f[3][0] : 'b';
  if (!strchr("bBlL", e)) return "Endian must be b, l, B or L (capital = signed)";
  s.little = (e == 'l' || e == 'L');
  s.sgn = (e == 'B' || e == 'L');
  s.scale = f[4].toFloat();
  if (s.scale == 0) return "Scale can't be 0";
  s.off = f[5].toFloat();
  s.name = nf > 6 && f[6].length() ? f[6] : String("Signal");
  s.unit = nf > 7 ? f[7] : String("");
  s.on = true;
  return "";
}

static void sigFrame(const twai_message_t &m) {
  for (auto &s : sigs) {
    if (!s.on || s.lid || s.id != m.identifier) continue;
    float v;
    if (sigExtract(s, m.data, m.data_length_code, v)) { s.val = v; s.at = millis(); }
  }
}
static bool sigFresh(const Sig &s) { return s.on && s.at && millis() - s.at < 1500; }

static void trackFrame(const twai_message_t &m) {
  sigFrame(m);
  if (!monitorOn) return;
  uint8_t n = m.data_length_code > 8 ? 8 : m.data_length_code;
  uint32_t now = millis();
  for (int i = 0; i < idCount; i++) {
    IdEntry &e = ids[i];
    if (e.id != m.identifier || e.ext != (bool)m.extd) continue;
    for (int b = 0; b < n; b++) {
      if (b >= e.dlc || e.data[b] != m.data[b]) { e.chg |= 1 << b; e.mk |= 1 << b; }
      if (m.data[b] < e.lo[b]) e.lo[b] = m.data[b];
      if (m.data[b] > e.hi[b]) e.hi[b] = m.data[b];
    }
    memcpy(e.data, m.data, n);
    e.dlc = n;
    e.count++;
    e.last = now;
    return;
  }
  if (idCount < MAX_IDS) {
    IdEntry &e = ids[idCount++];
    e.id = m.identifier;
    e.ext = m.extd;
    e.dlc = n;
    memcpy(e.data, m.data, n);
    memcpy(e.lo, m.data, n);
    memcpy(e.hi, m.data, n);
    e.chg = e.mk = 0;
    e.count = 1;
    e.first = e.last = now;
  }
}

static void sniffMark() {
  for (int i = 0; i < idCount; i++) {
    ids[i].mk = 0;
    memcpy(ids[i].lo, ids[i].data, 8);
    memcpy(ids[i].hi, ids[i].data, 8);
  }
}

// =====================================================================
//  TWAI (CAN) driver
// =====================================================================
static bool installTwai(twai_mode_t mode, int rateIdx) {
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)sInt(K_CAN_TX), (gpio_num_t)sInt(K_CAN_RX), mode);
  g.rx_queue_len = 128;
  g.tx_queue_len = 8;
  twai_timing_config_t t500 = TWAI_TIMING_CONFIG_500KBITS();
  twai_timing_config_t t250 = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, rateIdx == 0 ? &t500 : &t250, &f) != ESP_OK) return false;
  if (twai_start() != ESP_OK) { twai_driver_uninstall(); return false; }
  return true;
}

static void uninstallTwai() { twai_stop(); twai_driver_uninstall(); }

static bool probeRate(int rateIdx) {
  if (!installTwai(TWAI_MODE_LISTEN_ONLY, rateIdx)) return false;
  uint32_t got = 0, t0 = millis();
  twai_message_t m;
  while (millis() - t0 < 1500)
    if (twai_receive(&m, pdMS_TO_TICKS(50)) == ESP_OK) got++;
  uninstallTwai();
  Serial.printf("  %s: %lu frames\n", rateIdx == 0 ? "500k" : "250k", (unsigned long)got);
  return got > 0;
}

static void checkBusHealth() {
  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();
  twai_status_info_t st;
  if (twai_get_status_info(&st) != ESP_OK) return;
  if (st.state == TWAI_STATE_BUS_OFF) twai_initiate_recovery();
  else if (st.state == TWAI_STATE_STOPPED) twai_start();
}

// =====================================================================
//  ISO-TP request / response (ISO 15765-2)
// =====================================================================
static void sendFrame(uint32_t id, const uint8_t *d) {
  twai_message_t m = {};
  m.identifier = id;
  m.data_length_code = 8;
  memcpy(m.data, d, 8);
  twai_transmit(&m, pdMS_TO_TICKS(50));
}

// Send a request to CAN ID tx and collect the reply from rx (rx = 0: any OBD responder
// 7E8-7EF). Returns payload length, 0 on timeout. Single and multi-frame replies.
static int isoRequest(uint32_t tx, uint32_t rx, const uint8_t *req, uint8_t len, uint8_t *resp, int maxLen, uint32_t timeoutMs) {
  if (len > 7) return 0;
  twai_message_t m;
  while (twai_receive(&m, 0) == ESP_OK) trackFrame(m);

  uint8_t sf[8] = {0};
  sf[0] = len;
  memcpy(sf + 1, req, len);
  sendFrame(tx, sf);

  int total = 0, got = 0;
  uint8_t seq = 1;
  uint32_t deadline = millis() + timeoutMs;

  while ((int32_t)(deadline - millis()) > 0) {
    if (twai_receive(&m, pdMS_TO_TICKS(10)) != ESP_OK) continue;
    trackFrame(m);
    if (m.extd) continue;
    if (rx) { if (m.identifier != rx) continue; }
    else if (m.identifier < 0x7E8 || m.identifier > 0x7EF) continue;
    lastRespId = m.identifier;

    uint8_t pci = m.data[0] >> 4;
    if (pci == 0) {
      int n = m.data[0] & 0x0F;
      if (n > 7) n = 7;
      if (n >= 3 && m.data[1] == 0x7F && m.data[3] == 0x78) {   // response pending
        deadline = millis() + 5000;
        continue;
      }
      n = min(n, maxLen);
      memcpy(resp, m.data + 1, n);
      return n;
    } else if (pci == 1) {
      total = ((m.data[0] & 0x0F) << 8) | m.data[1];
      got = min(6, maxLen);
      memcpy(resp, m.data + 2, got);
      uint8_t fc[8] = {0x30, 0x00, 0x00, 0, 0, 0, 0, 0};
      sendFrame(rx ? tx : m.identifier - 8, fc);
      seq = 1;
      deadline = millis() + timeoutMs;
    } else if (pci == 2 && total > 0) {
      if ((m.data[0] & 0x0F) != seq) return 0;
      seq = (seq + 1) & 0x0F;
      int n = min(7, total - got);
      n = min(n, maxLen - got);
      if (n > 0) { memcpy(resp + got, m.data + 1, n); got += n; }
      if (got >= total || got >= maxLen) return got;
    }
  }
  return 0;
}

// Request to the engine ECU (or functional 7DF until it has been found)
static int obdRequest(const uint8_t *req, uint8_t len, uint8_t *resp, int maxLen, uint32_t timeoutMs) {
  return isoRequest(ecuRespId ? ecuRespId - 8 : OBD_FUNC_ID, ecuRespId, req, len, resp, maxLen, timeoutMs);
}

static const char *nrcText(uint8_t c) {
  switch (c) {
    case 0x10: return "general reject";
    case 0x11: return "service not supported";
    case 0x12: return "sub-function not supported";
    case 0x13: return "bad message length";
    case 0x22: return "conditions not correct (try key on, engine off)";
    case 0x31: return "request out of range";
    case 0x33: return "security access denied";
    case 0x35: return "invalid key";
    case 0x36: return "too many attempts";
    case 0x37: return "time delay not expired";
    case 0x7E: return "not supported in this session";
    case 0x7F: return "service not supported in this session";
    default:   return "unknown";
  }
}

// =====================================================================
//  DTC descriptions (generic SAE meanings for codes seen on the 390)
// =====================================================================
struct DtcDesc { const char *code, *desc; };
static const DtcDesc DTCS[] = {
  {"P0030", "O2 sensor heater control circuit"},
  {"P0031", "O2 sensor heater circuit low"},
  {"P0032", "O2 sensor heater circuit high"},
  {"P0105", "MAP sensor circuit"},
  {"P0106", "MAP sensor range/performance"},
  {"P0107", "MAP sensor circuit low"},
  {"P0108", "MAP sensor circuit high"},
  {"P0110", "Intake air temp sensor circuit"},
  {"P0112", "Intake air temp sensor low"},
  {"P0113", "Intake air temp sensor high"},
  {"P0115", "Coolant temp sensor circuit"},
  {"P0116", "Coolant temp sensor range/performance"},
  {"P0117", "Coolant temp sensor low"},
  {"P0118", "Coolant temp sensor high"},
  {"P0120", "Throttle position sensor circuit"},
  {"P0121", "Throttle position sensor range/performance"},
  {"P0122", "Throttle position sensor low"},
  {"P0123", "Throttle position sensor high"},
  {"P0130", "O2 sensor circuit"},
  {"P0131", "O2 sensor low voltage"},
  {"P0132", "O2 sensor high voltage"},
  {"P0133", "O2 sensor slow response"},
  {"P0134", "O2 sensor no activity"},
  {"P0171", "System too lean"},
  {"P0172", "System too rich"},
  {"P0201", "Injector circuit"},
  {"P0217", "Engine overheat"},
  {"P0230", "Fuel pump relay circuit"},
  {"P0300", "Misfire detected"},
  {"P0335", "Crank position sensor (pickup) circuit"},
  {"P0336", "Crank position sensor range/performance"},
  {"P0351", "Ignition coil circuit"},
  {"P0480", "Cooling fan control circuit"},
  {"P0500", "Vehicle speed sensor"},
  {"P0505", "Idle control system"},
  {"P0506", "Idle speed lower than expected"},
  {"P0507", "Idle speed higher than expected"},
  {"P0560", "System voltage"},
  {"P0562", "System voltage low"},
  {"P0563", "System voltage high"},
  {"P0601", "ECU memory checksum"},
  {"P0650", "FI warning lamp circuit"},
};

static const char *dtcDesc(const char *code) {
  for (auto &d : DTCS) if (!strcmp(d.code, code)) return d.desc;
  return "";
}

static void dtcStr(uint8_t a, uint8_t b, char *s) {
  const char t[] = "PCBU";
  sprintf(s, "%c%X%X%02X", t[a >> 6], (a >> 4) & 0x03, a & 0x0F, b);
}

// =====================================================================
//  PID helpers
// =====================================================================
static bool isSupp(uint8_t pid) {
  if (pid == 0 || pid > 0xC0) return false;
  int i = pid - 1;
  return suppBits[i / 8] & (0x80 >> (i % 8));
}

static void readSupported() {
  memset(suppBits, 0, sizeof(suppBits));
  for (int base = 0; base <= 0xA0; base += 0x20) {
    uint8_t rq[2] = {0x01, (uint8_t)base};
    uint8_t r[8];
    int n = obdRequest(rq, 2, r, sizeof(r), 500);
    if (n < 6 || r[0] != 0x41) break;
    for (int b = 0; b < 4; b++) suppBits[base / 8 + b] = r[2 + b];
    if (!(r[5] & 1)) break;
  }
}

static float decodePid(uint8_t pid, uint8_t A, uint8_t B) {
  switch (pid) {
    case 0x04: case 0x11: return A * 100.0f / 255.0f;
    case 0x05: case 0x0F: return A - 40.0f;
    case 0x06: case 0x07: return (A - 128) * 100.0f / 128.0f;
    case 0x0B: case 0x0D: return A;
    case 0x0C: return (A * 256 + B) / 4.0f;
    case 0x0E: return A / 2.0f - 64.0f;
    case 0x14: return A / 200.0f;
    case 0x1F: case 0x21: case 0x31: return A * 256 + B;
    case 0x30: return A;
    case 0x42: return (A * 256 + B) / 1000.0f;
  }
  return A;
}

static bool readPid(uint8_t pid, float &v, uint32_t to = 250) {
  uint8_t rq[2] = {0x01, pid};
  uint8_t r[8];
  int n = obdRequest(rq, 2, r, sizeof(r), to);
  if (n < 3 || r[0] != 0x41 || r[1] != pid) return false;
  v = decodePid(pid, r[2], n > 3 ? r[3] : 0);
  return true;
}

// KTM extended RPM: KWP2000 ReadDataByLocalIdentifier 0x21, local ID 0x21
static bool readKtmRpm(float &rpm, uint32_t to = 250) {
  uint8_t rq[2] = {0x21, 0x21};
  uint8_t r[16];
  int n = obdRequest(rq, 2, r, sizeof(r), to);
  if (n >= 4 && r[0] == 0x61 && r[1] == 0x21) {
    rpm = (r[2] * 256 + r[3]) / sFlt(K_RPM_DIV);
    return true;
  }
  return false;
}

static bool readVbat(float &v) {
  int pin = sInt(K_VBAT_PIN);
  if (pin < 32 || pin > 39) return false;
  uint32_t mv = 0;
  for (int i = 0; i < 4; i++) mv += analogReadMilliVolts(pin);
  v = (mv / 4) / 1000.0f * sFlt(K_VBAT_MULT);
  return true;
}

// =====================================================================
//  Live data + logging
// =====================================================================
enum { L_RPM, L_TPS, L_MAP, L_SPD, L_ECT, L_IAT, L_ADV, L_STFT, L_LTFT, L_O2, L_VBAT,
       L_GEAR, L_C1, L_C2, L_C3, L_C4, L_N };
static const char *LKEY[L_N]   = {"rpm", "tps", "map", "spd", "ect", "iat", "adv", "stft", "ltft", "o2", "vbat",
                                  "gear", "c1", "c2", "c3", "c4"};
static const uint8_t LPID[L_N] = {0x00, 0x11, 0x0B, 0x0D, 0x05, 0x0F, 0x0E, 0x06, 0x07, 0x14, 0x00,
                                  0, 0, 0, 0, 0};
static const float LSCALE[L_N] = {1, 10, 1, 1, 1, 1, 10, 10, 10, 1000, 100, 1, 10, 10, 10, 10};
static const uint8_t LDP[L_N]  = {0, 1, 0, 0, 0, 0, 1, 1, 1, 3, 2, 0, 2, 2, 2, 2};

struct LiveData { uint32_t ms; float v[L_N]; uint32_t ok; int fi; int dtc; };
static LiveData live = {0, {0}, 0, -1, -1};
static uint32_t lastLivePoll = 0, lastFiPoll = 0;
static int liveFailStreak = 0;

// Peak hold (max RPM / speed / coolant since reset)
static float pkRpm = 0, pkSpd = 0, pkEct = -100;

// Gear indicator: nearest configured rpm-per-km/h ratio
static float gearRatio[6];
static void loadGears() {
  String s = sStr(K_GEARS);
  int g = 0, start = 0;
  for (int i = 0; i <= (int)s.length() && g < 6; i++)
    if (i == (int)s.length() || s[i] == ',') { gearRatio[g++] = s.substring(start, i).toFloat(); start = i + 1; }
  while (g < 6) gearRatio[g++] = 0;
}
static int calcGear(float rpm, float spd) {
  if (spd < 8 || rpm < 1500) return 0;
  float r = rpm / spd, bestErr = 1e9;
  int best = 0;
  for (int g = 0; g < 6; g++) {
    if (gearRatio[g] <= 0) continue;
    float e = fabsf(r - gearRatio[g]) / gearRatio[g];
    if (e < bestErr) { bestErr = e; best = g + 1; }
  }
  return bestErr < 0.12f ? best : 0;   // > 12% off any gear = clutch in / slipping
}

// Log buffer, allocated when logging starts: up to LOG_BIG samples (~60 KB) if that still leaves
// LOG_SPARE bytes for Wi-Fi / Bluetooth, else LOG_N (~24 KB).
#define LOG_N 600
#define LOG_BIG 1500
#define LOG_SPARE 60000
struct LogRec { uint32_t ms; int16_t v[L_N]; uint32_t ok; };
static LogRec *logBuf = nullptr;
static int logMax = LOG_N;
static bool logAlloc() {
  if (logBuf) return true;
  const int sizes[] = {LOG_BIG, LOG_N};
  for (int n : sizes) {
    if (ESP.getFreeHeap() < (size_t)n * sizeof(LogRec) + LOG_SPARE && n != LOG_N) continue;
    logBuf = (LogRec *)calloc(n, sizeof(LogRec));
    if (logBuf) { logMax = n; return true; }
  }
  return false;
}
static int logHead = 0, logCount = 0;
static bool logging = false;
static uint32_t lastLogMs = 0, logStartMs = 0;

// Fast values are read every cycle; slow ones (temps, trims, O2) one per cycle in turn.
// ~5 ECU requests per cycle keeps RPM/throttle at 6-10 updates a second.
static const int FAST[] = {L_TPS, L_MAP, L_SPD};
static const int SLOW[] = {L_ECT, L_IAT, L_ADV, L_STFT, L_LTFT, L_O2};
static int slowIdx = 0;
#define LIVE_TO 80   // ms per live request; the Duke ECU answers in ~5-15 ms

static void pollOne(int i, uint32_t &ok, bool &any) {
  if (!LPID[i] || !isSupp(LPID[i])) { ok &= ~(1 << i); return; }
  float v;
  if (readPid(LPID[i], v, LIVE_TO)) { live.v[i] = v; ok |= 1 << i; any = true; }
  else ok &= ~(1 << i);
}

// Custom signals: broadcast ones are decoded as frames arrive; local-ID ones are read here.
static void pollSigs(uint32_t &ok) {
  int lastLid = -1, n = 0;
  uint8_t r[64];
  for (int i = 0; i < 4; i++) {
    Sig &s = sigs[i];
    if (s.on && s.lid && ecuFound) {
      if (s.lidNum != lastLid) {
        uint8_t rq[2] = {0x21, s.lidNum};
        n = obdRequest(rq, 2, r, sizeof(r), LIVE_TO);
        if (n < 2 || r[0] != 0x61) n = 0;
        lastLid = s.lidNum;
      }
      float v;
      if (n && sigExtract(s, r + 2, n - 2, v)) { s.val = v; s.at = millis(); }
    }
    if (sigFresh(s)) { live.v[L_C1 + i] = s.val; ok |= 1UL << (L_C1 + i); }
    else ok &= ~(1UL << (L_C1 + i));
  }
}

static void pollLive() {
  uint32_t ok = live.ok;
  pollSigs(ok);          // broadcast signals work even when the ECU doesn't answer
  if (!ecuFound) { live.ok = ok & ((1UL << L_C1) | (1UL << L_C2) | (1UL << L_C3) | (1UL << L_C4)); live.ms = millis(); return; }
  bool any = false;
  float v;
  if (ktmRpmOk) {
    if (readKtmRpm(v, LIVE_TO)) { live.v[L_RPM] = v; ok |= 1 << L_RPM; any = true; }
    else ok &= ~(1 << L_RPM);
  }
  for (int i : FAST) pollOne(i, ok, any);
  pollOne(SLOW[slowIdx], ok, any);
  slowIdx = (slowIdx + 1) % (int)(sizeof(SLOW) / sizeof(SLOW[0]));

  if (readVbat(v)) { live.v[L_VBAT] = v; ok |= 1 << L_VBAT; }
  else ok &= ~(1 << L_VBAT);

  if ((ok & (1 << L_RPM)) && (ok & (1 << L_SPD))) {
    live.v[L_GEAR] = calcGear(live.v[L_RPM], live.v[L_SPD]);
    ok |= 1UL << L_GEAR;
  } else ok &= ~(1UL << L_GEAR);

  if ((ok & (1 << L_RPM)) && live.v[L_RPM] > pkRpm) pkRpm = live.v[L_RPM];
  if ((ok & (1 << L_SPD)) && live.v[L_SPD] > pkSpd) pkSpd = live.v[L_SPD];
  if ((ok & (1 << L_ECT)) && live.v[L_ECT] > pkEct) pkEct = live.v[L_ECT];

  if (millis() - lastFiPoll > 5000) {
    lastFiPoll = millis();
    uint8_t rq[2] = {0x01, 0x01};
    uint8_t r[8];
    int n = obdRequest(rq, 2, r, sizeof(r), 300);
    if (n >= 3 && r[0] == 0x41 && r[1] == 0x01) { live.fi = (r[2] & 0x80) ? 1 : 0; live.dtc = r[2] & 0x7F; }
  }

  live.ok = ok;
  live.ms = millis();
  liveFailStreak = any ? 0 : liveFailStreak + 1;
  if (liveFailStreak >= 15) {           // ECU went away (key off)
    ecuFound = false;
    ecuRespId = 0;
    liveFailStreak = 0;
    live.ok = ok & ((1UL << L_VBAT) | (1UL << L_C1) | (1UL << L_C2) | (1UL << L_C3) | (1UL << L_C4));
  }
}

static String liveJson() {
  String o = "{\"t\":\"live\",\"ms\":" + String(live.ms) + ",\"ecu\":" + (ecuFound ? "true" : "false");
  for (int i = 0; i < L_N; i++) {
    o += ",\"" + String(LKEY[i]) + "\":";
    o += (live.ok & (1UL << i)) ? fmtF(live.v[i], LDP[i]) : String("null");
  }
  o += ",\"fi\":" + String(live.fi) + ",\"dtc\":" + String(live.dtc);
  o += ",\"pk\":{\"rpm\":" + String((int)pkRpm) + ",\"spd\":" + String((int)pkSpd) +
       ",\"ect\":" + (pkEct > -100 ? String((int)pkEct) : String("null")) + "}";
  o += ",\"log\":" + String(logging ? "true" : "false") + "}";
  return o;
}

static void logSample() {
  if (!logAlloc()) { logging = false; return; }
  LogRec &r = logBuf[logHead];
  r.ms = millis() - logStartMs;
  r.ok = live.ok;
  for (int i = 0; i < L_N; i++) {
    float s = live.v[i] * LSCALE[i];
    if (s > 32767) s = 32767;
    if (s < -32768) s = -32768;
    r.v[i] = (int16_t)s;
  }
  logHead = (logHead + 1) % logMax;
  if (logCount < logMax) logCount++;
}

static String logHeader() {
  String h = "seconds,rpm,tps_pct,map_kpa,speed_kmh,coolant_c,intake_c,advance_deg,stft_pct,ltft_pct,o2_v,battery_v,gear";
  for (int i = 0; i < 4; i++) {
    String n = sigs[i].on ? sigs[i].name : String("signal") + (i + 1);
    n.replace(",", " ");
    if (sigs[i].on && sigs[i].unit.length()) n += "_" + sigs[i].unit;
    h += "," + n;
  }
  return h + "\n";
}

static String logRow(int idx) {
  if (!logBuf) return "";
  const LogRec &r = logBuf[idx];
  String s = String((float)(r.ms / 1000.0f), 1U);
  for (int i = 0; i < L_N; i++) {
    s += ',';
    if (r.ok & (1UL << i)) s += String((float)(r.v[i] / LSCALE[i]), (unsigned int)min((int)LDP[i], LSCALE[i] >= 100 ? 3 : 1));
  }
  s += '\n';
  return s;
}

static int logIndex(int n) { return (logHead - logCount + n + logMax) % logMax; }

// =====================================================================
//  ECU connection
// =====================================================================
static void ecuTask() {
  if (ecuFound || millis() - lastPing < 3000) return;
  lastPing = millis();
  ecuRespId = 0;
  uint8_t rq[2] = {0x01, 0x00};
  uint8_t r[8];
  int n = obdRequest(rq, 2, r, sizeof(r), 800);
  if (n >= 2 && r[0] == 0x41) {
    ecuRespId = lastRespId;
    ecuFound = true;
    readSupported();
    float rpm;
    ktmRpmOk = readKtmRpm(rpm);
    Serial.printf("ECU found on %03lX, KTM RPM %s\n", (unsigned long)ecuRespId, ktmRpmOk ? "yes" : "no");
  }
}

// =====================================================================
//  Commands (shared by web, app, serial)
// =====================================================================
enum Chan : uint8_t { CH_SERIAL, CH_WEB, CH_BT };
static uint32_t btAppAt = 0;   // last time the OBDuke app talked over Bluetooth

static String dtcListJson(uint8_t mode) {
  uint8_t rq[1] = {mode};
  uint8_t r[64];
  int n = obdRequest(rq, 1, r, sizeof(r), 1500);
  if (n == 0 || r[0] != mode + 0x40) return "null";
  int idx, count;
  if (n >= 2 && (n - 2) == r[1] * 2) { count = r[1]; idx = 2; }
  else { count = (n - 1) / 2; idx = 1; }
  String o = "[";
  bool first = true;
  for (int i = 0; i < count; i++) {
    uint8_t a = r[idx + i * 2], b = r[idx + i * 2 + 1];
    if (!a && !b) continue;
    char s[8];
    dtcStr(a, b, s);
    if (!first) o += ",";
    o += "{\"code\":\"" + String(s) + "\",\"desc\":" + q(dtcDesc(s)) + "}";
    first = false;
  }
  return o + "]";
}

static String cmdCodes() {
  if (!ecuFound) return errJson("codes", "ECU not connected. Key on?");
  String o = "{\"t\":\"codes\",\"ok\":true";
  uint8_t rq[2] = {0x01, 0x01};
  uint8_t r[8];
  int n = obdRequest(rq, 2, r, sizeof(r), 500);
  if (n >= 3 && r[0] == 0x41 && r[1] == 0x01) {
    o += ",\"fi\":" + String((r[2] & 0x80) ? "true" : "false") + ",\"count\":" + String(r[2] & 0x7F);
    live.fi = (r[2] & 0x80) ? 1 : 0;
    live.dtc = r[2] & 0x7F;
  }
  float v;
  if (isSupp(0x21) && readPid(0x21, v, 400)) o += ",\"km_fi\":" + String((int)v);
  if (isSupp(0x31) && readPid(0x31, v, 400)) o += ",\"km_clr\":" + String((int)v);
  if (isSupp(0x30) && readPid(0x30, v, 400)) o += ",\"warmups\":" + String((int)v);
  o += ",\"stored\":" + dtcListJson(0x03);
  o += ",\"pending\":" + dtcListJson(0x07);
  return o + "}";
}

static String cmdClear(const String &arg) {
  if (arg != "yes") return errJson("clear", "Send 'clear yes' to confirm");
  if (!ecuFound) return errJson("clear", "ECU not connected. Key on?");
  uint8_t rq[1] = {0x04};
  uint8_t r[8];
  int n = obdRequest(rq, 1, r, sizeof(r), 3000);
  if (n == 0) return errJson("clear", "No response - clear may not have worked");
  if (r[0] == 0x7F) return errJson("clear", String("ECU refused: ") + (n >= 3 ? nrcText(r[2]) : "?"));
  if (r[0] != 0x44) return errJson("clear", "Unexpected reply " + toHex(r, n));
  live.fi = 0;
  live.dtc = 0;
  return "{\"t\":\"clear\",\"ok\":true,\"msg\":\"Codes cleared. Cycle the key and check the FI light.\"}";
}

static String cmdFreeze() {
  if (!ecuFound) return errJson("freeze", "ECU not connected");
  uint8_t rq[3] = {0x02, 0x02, 0x00};
  uint8_t r[16];
  int n = obdRequest(rq, 3, r, sizeof(r), 600);
  if (n < 5 || r[0] != 0x42 || (!r[3] && !r[4]))
    return "{\"t\":\"freeze\",\"ok\":true,\"code\":null,\"msg\":\"No freeze frame stored\"}";
  char s[8];
  dtcStr(r[3], r[4], s);
  String o = "{\"t\":\"freeze\",\"ok\":true,\"code\":\"" + String(s) + "\",\"desc\":" + q(dtcDesc(s)) + ",\"vals\":{";
  const uint8_t pids[] = {0x05, 0x0F, 0x11, 0x0B, 0x0D, 0x0E, 0x06, 0x07, 0x14};
  const char *keys[] = {"ect", "iat", "tps", "map", "spd", "adv", "stft", "ltft", "o2"};
  bool first = true;
  for (int i = 0; i < 9; i++) {
    uint8_t fq[3] = {0x02, pids[i], 0x00};
    int m = obdRequest(fq, 3, r, sizeof(r), 300);
    if (m >= 4 && r[0] == 0x42 && r[1] == pids[i]) {
      if (!first) o += ",";
      o += "\"" + String(keys[i]) + "\":" + fmtF(decodePid(pids[i], r[3], m > 4 ? r[4] : 0), 1);
      first = false;
    }
  }
  return o + "}}";
}

static String cmdPids() {
  if (!ecuFound) return errJson("pids", "ECU not connected");
  readSupported();
  String o = "{\"t\":\"pids\",\"ok\":true,\"list\":[";
  bool first = true;
  for (int p = 1; p <= 0xC0; p++) {
    if (!isSupp(p)) continue;
    char b[6];
    snprintf(b, sizeof(b), "\"%02X\"", p);
    if (!first) o += ",";
    o += b;
    first = false;
  }
  o += "],\"ktm_rpm\":" + String(ktmRpmOk ? "true" : "false") + "}";
  return o;
}

static String cmdVin() {
  uint8_t rq[2] = {0x09, 0x02};
  uint8_t r[40];
  int n = obdRequest(rq, 2, r, sizeof(r), 1500);
  if (n == 0 || r[0] != 0x49) return errJson("vin", "No VIN response");
  String vin;
  int start = n - 17;
  if (start < 2) start = 2;
  for (int i = start; i < n; i++) if (r[i] >= 0x20 && r[i] < 0x7F) vin += (char)r[i];
  return "{\"t\":\"vin\",\"ok\":true,\"vin\":" + q(vin) + "}";
}

static String itemJson(const String &k, const uint8_t *d, int n) {
  return "{\"k\":" + q(k) + ",\"hex\":\"" + toHex(d, n) + "\",\"ascii\":" + q(toAscii(d, n)) + "}";
}

static String cmdIdent() {
  String o = "{\"t\":\"ident\",\"ok\":true,\"items\":[";
  bool first = true;
  uint8_t r[64];
  const struct { uint8_t pid; const char *name; } m9[] = {
    {0x02, "VIN"}, {0x04, "Calibration ID"}, {0x06, "CVN"}, {0x0A, "ECU name"}};
  for (auto &e : m9) {
    uint8_t rq[2] = {0x09, e.pid};
    int n = obdRequest(rq, 2, r, sizeof(r), 800);
    if (n >= 3 && r[0] == 0x49) {
      if (!first) o += ",";
      o += itemJson(e.name, r + 2, n - 2);
      first = false;
    }
  }
  for (int opt = 0x80; opt <= 0x9F; opt++) {
    uint8_t rq[2] = {0x1A, (uint8_t)opt};
    int n = obdRequest(rq, 2, r, sizeof(r), 300);
    if (n >= 2 && r[0] == 0x5A) {
      char k[12];
      snprintf(k, sizeof(k), "KWP 1A%02X", opt);
      if (!first) o += ",";
      o += itemJson(k, r + 2, n - 2);
      first = false;
    }
  }
  const uint16_t dids[] = {0xF187, 0xF188, 0xF189, 0xF18A, 0xF18C, 0xF190,
                           0xF191, 0xF192, 0xF193, 0xF194, 0xF195, 0xF197};
  for (uint16_t did : dids) {
    uint8_t rq[3] = {0x22, (uint8_t)(did >> 8), (uint8_t)(did & 0xFF)};
    int n = obdRequest(rq, 3, r, sizeof(r), 300);
    if (n >= 3 && r[0] == 0x62) {
      char k[12];
      snprintf(k, sizeof(k), "UDS %04X", did);
      if (!first) o += ",";
      o += itemJson(k, r + 3, n - 3);
      first = false;
    }
  }
  return o + "]}";
}

static String cmdKtm() {
  String o = "{\"t\":\"ktm\",\"ok\":true,\"items\":[";
  const uint8_t lids[] = {0x21, 0x22, 0x23, 0x33};
  for (int i = 0; i < 4; i++) {
    uint8_t rq[2] = {0x21, lids[i]};
    uint8_t r[64];
    char k[8];
    snprintf(k, sizeof(k), "21%02X", lids[i]);
    int n = obdRequest(rq, 2, r, sizeof(r), 600);
    if (i) o += ",";
    if (n <= 0) o += "{\"k\":\"" + String(k) + "\",\"msg\":\"no reply\"}";
    else if (r[0] == 0x7F) o += "{\"k\":\"" + String(k) + "\",\"msg\":" + q(n >= 3 ? nrcText(r[2]) : "refused") + "}";
    else o += itemJson(k, r + 2, n - 2);
  }
  float rpm;
  o += "]";
  if (readKtmRpm(rpm)) o += ",\"rpm\":" + String((int)rpm);
  return o + "}";
}

static String cmdScan() {
  String o = "{\"t\":\"scan\",\"ok\":true,\"items\":[";
  bool first = true;
  for (int lid = 0; lid <= 0xFF; lid++) {
    uint8_t rq[2] = {0x21, (uint8_t)lid};
    uint8_t r[64];
    int n = obdRequest(rq, 2, r, sizeof(r), 200);
    if (n >= 2 && r[0] == 0x61) {
      char k[8];
      snprintf(k, sizeof(k), "21%02X", lid);
      if (!first) o += ",";
      o += itemJson(k, r + 2, n - 2);
      first = false;
    }
  }
  return o + "]}";
}

static String cmdDiscover() {
  const struct { uint8_t req[4]; uint8_t len; const char *name; } probes[] = {
    {{0x10, 0x81}, 2, "KWP default session (10 81)"},
    {{0x10, 0x01}, 2, "UDS default session (10 01)"},
    {{0x3E, 0x00}, 2, "Tester present (3E)"},
    {{0x1A, 0x80}, 2, "KWP ECU identification (1A)"},
    {{0x21, 0x01}, 2, "KWP read local ID (21)"},
    {{0x22, 0xF1, 0x90}, 3, "UDS read data ID (22)"},
    {{0x18, 0x00, 0xFF, 0x00}, 4, "KWP read DTCs by status (18)"},
    {{0x19, 0x02, 0xFF}, 3, "UDS read DTCs by status (19)"},
    {{0x27, 0x01}, 2, "Security access seed (27)"},
    // Service ID only, no parameters: the ECU answers "bad length" if it supports
    // the service, or "not supported" if not. Nothing is actuated.
    {{0x30}, 1, "I/O control / actuator tests (30)"},
    {{0x2F}, 1, "UDS I/O control (2F)"},
    {{0x31}, 1, "Routine control (31)"},
  };
  String o = "{\"t\":\"discover\",\"ok\":true,\"items\":[";
  bool first = true;
  for (auto &p : probes) {
    uint8_t r[64];
    int n = obdRequest(p.req, p.len, r, sizeof(r), 400);
    String res;
    if (n == 0) res = "no reply";
    else if (r[0] == 0x7F) res = String("NRC ") + toHex(r + 2, 1) + " " + (n >= 3 ? nrcText(r[2]) : "");
    else res = "OK: " + toHex(r, n);
    if (!first) o += ",";
    o += "{\"k\":" + q(p.name) + ",\"msg\":" + q(res) + "}";
    first = false;
  }
  return o + "]}";
}

static int parseHex(const String &s, uint8_t *out, int maxLen);

static String idStr(uint32_t id, bool ext) {
  char b[10];
  snprintf(b, sizeof(b), ext ? "%08lX" : "%03lX", (unsigned long)id);
  return b;
}

static String cmdMonitor(uint32_t ms) {
  bool was = monitorOn;
  if (!was) idCount = 0;
  sniffMark();
  monitorOn = true;
  uint32_t t0 = millis();
  twai_message_t m;
  while (millis() - t0 < ms) {
    if (twai_receive(&m, pdMS_TO_TICKS(20)) == ESP_OK) trackFrame(m);
  }
  monitorOn = was;
  String o = "{\"t\":\"monitor\",\"ok\":true,\"ms\":" + String(ms) + ",\"ids\":[";
  for (int i = 0; i < idCount; i++) {
    if (i) o += ",";
    o += "{\"id\":\"" + idStr(ids[i].id, ids[i].ext) + "\",\"n\":" + String(ids[i].count) + ",\"data\":\"" +
         toHex(ids[i].data, ids[i].dlc) + "\",\"chg\":" + (ids[i].mk ? "true" : "false") + "}";
  }
  return o + "]}";
}

// Live bus sniffer: "sniff start", "sniff" (read table), "sniff mark", "sniff stop"
static String cmdSniff(const String &a) {
  if (a == "start") { idCount = 0; monitorOn = true; sniffStart = millis(); }
  else if (a == "stop") monitorOn = false;
  else if (a == "mark") sniffMark();
  else if (a == "clear") { idCount = 0; sniffStart = millis(); }
  sniffPolled = millis();
  String o = "{\"t\":\"sniff\",\"ok\":true,\"on\":" + String(monitorOn ? "true" : "false") +
             ",\"ms\":" + String(monitorOn ? millis() - sniffStart : 0) + ",\"full\":" + (idCount >= MAX_IDS ? "true" : "false") + ",\"ids\":[";
  for (int i = 0; i < idCount; i++) {
    IdEntry &e = ids[i];
    float hz = e.last > e.first ? (e.count - 1) * 1000.0f / (e.last - e.first) : 0;
    if (i) o += ",";
    o += "{\"id\":\"" + idStr(e.id, e.ext) + "\",\"n\":" + String(e.count) + ",\"hz\":" + fmtF(hz, 1) +
         ",\"age\":" + String(millis() - e.last) + ",\"d\":\"" + toHex(e.data, e.dlc) + "\",\"c\":" + String(e.chg) +
         ",\"m\":" + String(e.mk) + ",\"lo\":\"" + toHex(e.lo, e.dlc) + "\",\"hi\":\"" + toHex(e.hi, e.dlc) + "\"}";
    e.chg = 0;
  }
  return o + "]}";
}

// ---------- other modules on the bus (ABS, dash...) ----------
struct Module { uint32_t tx, rx; String info; };
static Module mods[8];
static int modCount = 0;

static uint32_t modRx(uint32_t tx) {
  for (int i = 0; i < modCount; i++) if (mods[i].tx == tx) return mods[i].rx;
  if (ecuRespId && tx == ecuRespId - 8) return ecuRespId;
  return tx + 8;
}

static const char *modName(uint32_t rx) {
  if (rx == ecuRespId || (!ecuRespId && rx == 0x7E8)) return "Engine ECU";
  return "Module";
}

static String readModuleInfo(uint32_t tx, uint32_t rx) {
  const struct { uint8_t req[3]; uint8_t len; } probes[] = {
    {{0x1A, 0x9B}, 2}, {{0x1A, 0x91}, 2}, {{0x22, 0xF1, 0x87}, 3}, {{0x22, 0xF1, 0x8A}, 3}, {{0x09, 0x0A}, 2}};
  uint8_t r[64];
  for (auto &p : probes) {
    int n = isoRequest(tx, rx, p.req, p.len, r, sizeof(r), 200);
    if (n > p.len && r[0] == (p.req[0] | 0x40)) {
      String a = toAscii(r + p.len, n - p.len);
      a.replace(".", " ");
      a.trim();
      if (a.length() >= 3) return a;
    }
  }
  return "";
}

// Sends "tester present" (3E 00) to every 11-bit ID from 700 to 7F7 and records who answers.
// IDs already in use by normal broadcast traffic are skipped so we never impersonate a real frame.
static String cmdModules() {
  uint32_t seen[64];
  int ns = 0;
  twai_message_t m;
  uint32_t t0 = millis();
  while (millis() - t0 < 700) {
    if (twai_receive(&m, pdMS_TO_TICKS(10)) != ESP_OK) continue;
    trackFrame(m);
    if (m.extd) continue;
    bool dup = false;
    for (int i = 0; i < ns; i++) if (seen[i] == m.identifier) dup = true;
    if (!dup && ns < 64) seen[ns++] = m.identifier;
  }
  auto isSeen = [&](uint32_t id) { for (int i = 0; i < ns; i++) if (seen[i] == id) return true; return false; };

  modCount = 0;
  int skipped = 0;
  for (uint32_t tx = 0x700; tx <= 0x7F7 && modCount < 8; tx++) {
    if (tx == OBD_FUNC_ID) continue;
    if (isSeen(tx)) { skipped++; continue; }
    while (twai_receive(&m, 0) == ESP_OK) trackFrame(m);
    uint8_t sf[8] = {0x02, 0x3E, 0x00, 0, 0, 0, 0, 0};
    sendFrame(tx, sf);
    uint32_t dl = millis() + 30;
    while ((int32_t)(dl - millis()) > 0) {
      if (twai_receive(&m, pdMS_TO_TICKS(5)) != ESP_OK) continue;
      trackFrame(m);
      if (m.extd || m.identifier < 0x700 || m.identifier > 0x7FF || m.identifier == tx || isSeen(m.identifier)) continue;
      if ((m.data[0] >> 4) != 0) continue;
      if (m.data[1] == 0x7E || (m.data[1] == 0x7F && m.data[2] == 0x3E)) {
        bool dup = false;
        for (int i = 0; i < modCount; i++) if (mods[i].rx == m.identifier) dup = true;
        if (!dup) { mods[modCount].tx = tx; mods[modCount].rx = m.identifier; mods[modCount].info = ""; modCount++; }
        break;
      }
    }
  }
  for (int i = 0; i < modCount; i++) mods[i].info = readModuleInfo(mods[i].tx, mods[i].rx);

  String o = "{\"t\":\"modules\",\"ok\":true,\"skipped\":" + String(skipped) + ",\"list\":[";
  for (int i = 0; i < modCount; i++) {
    if (i) o += ",";
    o += "{\"tx\":\"" + idStr(mods[i].tx, false) + "\",\"rx\":\"" + idStr(mods[i].rx, false) +
         "\",\"name\":\"" + modName(mods[i].rx) + "\",\"info\":" + q(mods[i].info) + "}";
  }
  return o + "]}";
}

static bool parseTx(String a, uint32_t &tx) {
  a.trim();
  if (a.startsWith("@")) a = a.substring(1);
  if (!a.length() || a.length() > 3) return false;
  for (size_t i = 0; i < a.length(); i++) if (!isxdigit(a[i])) return false;
  tx = strtoul(a.c_str(), nullptr, 16);
  return tx > 0 && tx <= 0x7FF;
}

// Read codes from any module: tries KWP2000 (18), then UDS (19 02), then OBD mode 03.
static String cmdMcodes(const String &a) {
  uint32_t tx;
  if (!parseTx(a, tx)) return errJson("mcodes", "Give the module's request ID, e.g. mcodes 7E0");
  uint32_t rx = modRx(tx);
  uint8_t r[128];
  String tried;
  String head = "{\"t\":\"mcodes\",\"ok\":true,\"tx\":\"" + idStr(tx, false) + "\",\"rx\":\"" + idStr(rx, false) + "\"";

  uint8_t kwp[4] = {0x18, 0x02, 0xFF, 0x00};
  int n = isoRequest(tx, rx, kwp, 4, r, sizeof(r), 1500);
  if (n >= 2 && r[0] == 0x58) {
    String o = head + ",\"proto\":\"KWP2000\",\"list\":[";
    int cnt = r[1];
    for (int i = 0; i < cnt && 2 + i * 3 + 2 < n; i++) {
      const uint8_t *d = r + 2 + i * 3;
      char s[8];
      dtcStr(d[0], d[1], s);
      if (i) o += ",";
      o += "{\"code\":\"" + String(s) + "\",\"st\":\"" + toHex(d + 2, 1) + "\",\"desc\":" + q(dtcDesc(s)) + "}";
    }
    return o + "]}";
  }
  tried += "KWP 18: " + String(n == 0 ? "no reply" : (r[0] == 0x7F && n >= 3 ? nrcText(r[2]) : toHex(r, min(n, 6)).c_str()));

  uint8_t uds[3] = {0x19, 0x02, 0xFF};
  n = isoRequest(tx, rx, uds, 3, r, sizeof(r), 1500);
  if (n >= 3 && r[0] == 0x59) {
    String o = head + ",\"proto\":\"UDS\",\"list\":[";
    bool first = true;
    for (int p = 3; p + 3 < n; p += 4) {
      char s[8];
      dtcStr(r[p], r[p + 1], s);
      if (!first) o += ",";
      first = false;
      o += "{\"code\":\"" + String(s) + "-" + toHex(r + p + 2, 1) + "\",\"st\":\"" + toHex(r + p + 3, 1) + "\",\"desc\":" + q(dtcDesc(s)) + "}";
    }
    return o + "]}";
  }
  tried += " | UDS 19: " + String(n == 0 ? "no reply" : (r[0] == 0x7F && n >= 3 ? nrcText(r[2]) : toHex(r, min(n, 6)).c_str()));

  uint8_t obd[1] = {0x03};
  n = isoRequest(tx, rx, obd, 1, r, sizeof(r), 1500);
  if (n >= 1 && r[0] == 0x43) {
    String o = head + ",\"proto\":\"OBD\",\"list\":[";
    int idx = (n >= 2 && (n - 2) == r[1] * 2) ? 2 : 1;
    bool first = true;
    for (int p = idx; p + 1 < n; p += 2) {
      if (!r[p] && !r[p + 1]) continue;
      char s[8];
      dtcStr(r[p], r[p + 1], s);
      if (!first) o += ",";
      first = false;
      o += "{\"code\":\"" + String(s) + "\",\"st\":\"\",\"desc\":" + q(dtcDesc(s)) + "}";
    }
    return o + "]}";
  }
  tried += " | OBD 03: " + String(n == 0 ? "no reply" : (r[0] == 0x7F && n >= 3 ? nrcText(r[2]) : toHex(r, min(n, 6)).c_str()));
  return errJson("mcodes", "Module " + idStr(tx, false) + " didn't give codes. " + tried);
}

static String cmdMclear(const String &a) {
  int sp = a.indexOf(' ');
  uint32_t tx;
  if (sp < 0 || a.substring(sp + 1) != "yes" || !parseTx(a.substring(0, sp), tx))
    return errJson("mclear", "Send 'mclear <id> yes', e.g. mclear 7E0 yes");
  uint32_t rx = modRx(tx);
  uint8_t r[16];
  const struct { uint8_t req[4]; uint8_t len, ok; const char *name; } tries[] = {
    {{0x14, 0xFF, 0x00}, 3, 0x54, "KWP2000"}, {{0x14, 0xFF, 0xFF, 0xFF}, 4, 0x54, "UDS"}, {{0x04}, 1, 0x44, "OBD"}};
  String why;
  for (auto &t : tries) {
    int n = isoRequest(tx, rx, t.req, t.len, r, sizeof(r), 3000);
    if (n >= 1 && r[0] == t.ok)
      return "{\"t\":\"mclear\",\"ok\":true,\"tx\":\"" + idStr(tx, false) + "\",\"msg\":\"Codes cleared (" + t.name + "). Cycle the key.\"}";
    if (why.length()) why += " | ";
    why += String(t.name) + ": " + (n == 0 ? "no reply" : (r[0] == 0x7F && n >= 3 ? nrcText(r[2]) : "unexpected reply"));
  }
  return errJson("mclear", "Clear refused. " + why);
}

// Read-only request for the watch tool: [@ID] then hex. Only read services are allowed.
static String cmdPeek(String a) {
  a.trim();
  uint32_t tx = 0;
  if (a.startsWith("@")) {
    int sp = a.indexOf(' ');
    if (sp < 0 || !parseTx(a.substring(0, sp), tx)) return errJson("peek", "Use @ID then the request, e.g. @7E0 21 22");
    a = a.substring(sp + 1);
  }
  uint8_t rq[8];
  int len = parseHex(a, rq, 7);
  if (!len) return errJson("peek", "Give a request in hex, e.g. 21 22");
  const uint8_t okSvc[] = {0x01, 0x02, 0x09, 0x1A, 0x21, 0x22};
  bool allowed = false;
  for (uint8_t s : okSvc) if (rq[0] == s) allowed = true;
  if (!allowed) return errJson("peek", "Watch only allows read requests (01 02 09 1A 21 22)");
  uint8_t r[128];
  int n = tx ? isoRequest(tx, modRx(tx), rq, len, r, sizeof(r), 500) : obdRequest(rq, len, r, sizeof(r), 500);
  if (n == 0) return errJson("peek", "No reply");
  String o = "{\"t\":\"peek\",\"ok\":true,\"req\":\"" + toHex(rq, len) + "\",\"hex\":\"" + toHex(r, n) + "\"";
  if (r[0] == 0x7F && n >= 3) o += ",\"nrc\":" + q(nrcText(r[2]));
  return o + "}";
}

static String gearsJson() {
  String o = "[";
  for (int g = 0; g < 6; g++) { if (g) o += ","; o += fmtF(gearRatio[g], 1); }
  return o + "]";
}

static void saveSetting(int k);

// "gear" shows the ratios, "gear learn N" stores the current rpm/speed as gear N
static String cmdGear(const String &a) {
  if (a.startsWith("learn")) {
    int g = a.substring(5).toInt();
    if (g < 1 || g > 6) return errJson("gear", "Use gear learn 1-6");
    pollLive();   // fresh RPM + speed right now
    bool ok = (live.ok & (1 << L_RPM)) && (live.ok & (1 << L_SPD));
    if (!ok) return errJson("gear", "Need live RPM and speed from the ECU. Key on, engine running?");
    if (live.v[L_SPD] < 15) return errJson("gear", "Ride at 15 km/h or more, steady, clutch out");
    gearRatio[g - 1] = live.v[L_RPM] / live.v[L_SPD];
    String s;
    for (int i = 0; i < 6; i++) { if (i) s += ","; s += String(gearRatio[i], 1U); }
    sv[K_GEARS] = s;
    saveSetting(K_GEARS);
  } else if (a == "reset") {
    sv[K_GEARS] = SDEFS[K_GEARS].def;
    saveSetting(K_GEARS);
    loadGears();
  }
  return "{\"t\":\"gear\",\"ok\":true,\"ratios\":" + gearsJson() + "}";
}

static int parseHex(const String &s, uint8_t *out, int maxLen) {
  String h;
  for (size_t i = 0; i < s.length(); i++) if (isxdigit(s[i])) h += s[i];
  int n = 0;
  for (size_t i = 0; i + 1 < h.length() && n < maxLen; i += 2)
    out[n++] = strtoul(h.substring(i, i + 2).c_str(), nullptr, 16);
  return n;
}

static String cmdRaw(const String &arg) {
  if (!sBool(K_ALLOW_RAW)) return errJson("raw", "Raw commands are off (Settings > CAN / ECU)");
  uint8_t rq[8];
  int len = parseHex(arg, rq, 7);
  if (len == 0) return errJson("raw", "Give hex bytes, e.g. raw 22 F1 90");
  uint8_t r[128];
  int n = obdRequest(rq, len, r, sizeof(r), 1500);
  if (n == 0) return errJson("raw", "No reply");
  return "{\"t\":\"raw\",\"ok\":true,\"req\":\"" + toHex(rq, len) + "\",\"hex\":\"" + toHex(r, n) +
         "\",\"ascii\":" + q(toAscii(r, n)) + "}";
}

static String cmdStatus(Chan ch) {
  String o = "{\"t\":\"status\",\"ok\":true,\"fw\":\"" FW_VERSION "\"";
  o += ",\"ecu\":" + String(ecuFound ? "true" : "false");
  if (ecuFound) o += ",\"ecu_id\":\"" + idStr(ecuRespId, false) + "\"";
  o += ",\"can\":\"" + String(canRateIdx == 0 ? "500k" : "250k") + "\"";
  o += ",\"ktm_rpm\":" + String(ktmRpmOk ? "true" : "false");
  o += ",\"fi\":" + String(live.fi) + ",\"dtc\":" + String(live.dtc);
  o += ",\"uptime\":" + String(millis() / 1000);
  o += ",\"heap\":" + String(ESP.getFreeHeap());
  o += ",\"tracker\":" + String(trkStarted && SerialBT.hasClient() ? "true" : "false");
  {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char b[20];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    o += ",\"bt_mac\":\"" + String(b) + "\"";
  }
  o += ",\"wifi_clients\":" + String(WiFi.softAPgetStationNum());
  o += ",\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"";
  o += ",\"sta_ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("")) + "\"";
  o += ",\"log\":" + String(logging ? "true" : "false") + ",\"log_n\":" + String(logCount) + ",\"log_max\":" + String(logMax);
  o += ",\"units\":" + String(sInt(K_UNITS)) + ",\"live_ms\":" + String(sInt(K_LIVE_MS));
  o += ",\"temp_warn\":" + String(sInt(K_TEMP_WARN)) + ",\"rpm_warn\":" + String(sInt(K_RPM_WARN));
  o += ",\"vbat_low\":" + fmtF(sFlt(K_VBAT_LOW), 1);
  o += ",\"raw\":" + String(sBool(K_ALLOW_RAW) ? "true" : "false");
  o += ",\"bt_client\":" + String(trkStarted && SerialBT.hasClient() ? "true" : "false");
  o += ",\"bt_mode\":\"" + String(!(trkStarted && SerialBT.hasClient()) ? "none" : (btAppAt && millis() - btAppAt < 10000 ? "app" : "elm")) + "\"";
  o += ",\"gears\":" + gearsJson();
  o += ",\"sigs\":[";
  bool first = true;
  for (int i = 0; i < 4; i++) {
    if (!sigs[i].on) continue;
    if (!first) o += ",";
    first = false;
    o += "{\"k\":\"c" + String(i + 1) + "\",\"n\":" + q(sigs[i].name) + ",\"u\":" + q(sigs[i].unit) + "}";
  }
  o += "]";
  return o + "}";
}

static const char *typeName(SType t) {
  switch (t) {
    case S_STR: return "str"; case S_PASS: return "pass"; case S_INT: return "int";
    case S_BOOL: return "bool"; case S_FLOAT: return "float"; default: return "sel";
  }
}

static String cmdSettings() {
  String o = "{\"t\":\"settings\",\"ok\":true,\"fields\":[";
  for (int i = 0; i < K_COUNT; i++) {
    const SDef &d = SDEFS[i];
    if (i) o += ",";
    o += "{\"k\":\"" + String(d.key) + "\",\"l\":" + q(d.label) + ",\"g\":" + q(d.group) +
         ",\"t\":\"" + typeName(d.type) + "\",\"r\":" + (d.reboot ? "true" : "false") +
         ",\"min\":" + String(d.mn) + ",\"max\":" + String(d.mx);
    if (d.type == S_PASS) o += ",\"v\":\"\",\"set\":" + String(sv[i].length() ? "true" : "false");
    else o += ",\"v\":" + q(sv[i]);
    if (d.type == S_SEL) o += ",\"o\":" + q(d.opts);
    o += "}";
  }
  return o + "]}";
}

static String cmdSet(const String &args) {
  int sp = args.indexOf(' ');
  String key = sp < 0 ? args : args.substring(0, sp);
  String val = sp < 0 ? "" : args.substring(sp + 1);
  val.trim();
  int k = -1;
  for (int i = 0; i < K_COUNT; i++) if (key == SDEFS[i].key) { k = i; break; }
  if (k < 0) return errJson("set", "Unknown setting " + key);
  const SDef &d = SDEFS[k];

  switch (d.type) {
    case S_INT: case S_SEL: {
      if (!val.length()) return errJson("set", String(d.label) + ": value needed");
      for (size_t i = 0; i < val.length(); i++)
        if (!isdigit(val[i]) && !(i == 0 && val[i] == '-')) return errJson("set", String(d.label) + ": whole number needed");
      long n = val.toInt();
      if (n < d.mn || n > d.mx) return errJson("set", String(d.label) + ": must be " + String((int)d.mn) + "-" + String((int)d.mx));
      val = String(n);
      break;
    }
    case S_FLOAT: {
      float f = val.toFloat();
      if (!val.length() || f < d.mn || f > d.mx) return errJson("set", String(d.label) + ": must be " + String(d.mn, 1U) + "-" + String(d.mx, 1U));
      val = String(f, 2U);
      break;
    }
    case S_BOOL: {
      String v = val; v.toLowerCase();
      val = (v == "1" || v == "true" || v == "on" || v == "yes") ? "1" : "0";
      break;
    }
    case S_STR: case S_PASS: {
      if (val.length() < (size_t)d.mn && !(d.type == S_PASS && val.length() == 0))
        return errJson("set", String(d.label) + ": at least " + String((int)d.mn) + " characters");
      if (val.length() > (size_t)d.mx) return errJson("set", String(d.label) + ": max " + String((int)d.mx) + " characters");
      if (k == K_AP_PASS && val.length() > 0 && val.length() < 8) return errJson("set", "Hotspot password must be 8+ characters (or blank)");
      if (k == K_TRK_PIN) for (size_t i = 0; i < val.length(); i++) if (!isdigit(val[i])) return errJson("set", "PIN must be digits");
      if (k == K_HOSTNAME) for (size_t i = 0; i < val.length(); i++)
        if (!isalnum(val[i]) && val[i] != '-') return errJson("set", "Network name: letters, numbers and - only");
      break;
    }
  }
  if (k == K_AP_IP) {
    IPAddress ip;
    if (!ip.fromString(val) || ip[0] == 0 || ip[0] == 127 || ip[0] >= 224 || ip[3] == 0 || ip[3] == 255)
      return errJson("set", "Hotspot address must be a normal IP like 1.1.1.1 or 10.1.1.1");
  }
  if (k >= K_SIG1 && k <= K_SIG4) {
    Sig tmp;
    String e = sigParse(val, tmp);
    if (e.length()) return errJson("set", String("Signal ") + (k - K_SIG1 + 1) + ": " + e);
  }
  if (k == K_GEARS) {
    int commas = 0;
    for (size_t i = 0; i < val.length(); i++) {
      if (val[i] == ',') commas++;
      else if (!isdigit(val[i]) && val[i] != '.' && val[i] != ' ') return errJson("set", "Gear ratios: 6 numbers separated by commas");
    }
    if (commas != 5) return errJson("set", "Gear ratios: 6 numbers separated by commas");
  }
  if (k == K_VBAT_PIN && val.toInt() != 0 && (val.toInt() < 32 || val.toInt() > 39))
    return errJson("set", "Battery pin must be 32-39 (ADC1) or 0");
  if ((k == K_CAN_TX || k == K_CAN_RX || k == K_LED_PIN) && (val.toInt() >= 6 && val.toInt() <= 11))
    return errJson("set", "Pins 6-11 are used by the flash chip");

  sv[k] = val;
  saveSetting(k);
  if (k >= K_SIG1 && k <= K_SIG4) sigParse(val, sigs[k - K_SIG1]);
  if (k == K_GEARS) loadGears();
  return "{\"t\":\"set\",\"ok\":true,\"k\":\"" + key + "\",\"reboot\":" + (d.reboot ? "true" : "false") + "}";
}

// Log download in chunks for the Bluetooth app: "logrows <start> <count>"
static String cmdLogRows(const String &a) {
  int sp = a.indexOf(' ');
  int start = max(0L, a.toInt());
  int cnt = sp < 0 ? 40 : constrain(a.substring(sp + 1).toInt(), 1L, 80L);
  String rows;
  int n = 0;
  for (int i = start; i < logCount && n < cnt; i++, n++) rows += logRow(logIndex(i));
  String o = "{\"t\":\"logrows\",\"ok\":true,\"total\":" + String(logCount) + ",\"start\":" + String(start) + ",\"n\":" + String(n);
  if (start == 0) o += ",\"header\":" + q(logHeader());
  return o + ",\"rows\":" + q(rows) + "}";
}

static String cmdLog(const String &arg) {
  if (arg == "start") {
    if (!logAlloc()) return errJson("log", "Not enough memory to log");
    if (!logging) { logging = true; logStartMs = millis(); logHead = 0; logCount = 0; }
  } else if (arg == "stop") {
    logging = false;
  } else if (arg == "clear") {
    logHead = 0; logCount = 0;
  }
  return "{\"t\":\"log\",\"ok\":true,\"on\":" + String(logging ? "true" : "false") + ",\"n\":" + String(logCount) +
         ",\"max\":" + String(logMax) + ",\"ms\":" + String(sInt(K_LOG_MS)) + "}";
}


static String runCommand(String line, Chan ch) {
  line.trim();
  int sp = line.indexOf(' ');
  String c = sp < 0 ? line : line.substring(0, sp);
  String a = sp < 0 ? "" : line.substring(sp + 1);
  c.toLowerCase();
  a.trim();

  if (c == "hello" || c == "status") return cmdStatus(ch);
  if (c == "codes")    return cmdCodes();
  if (c == "clear")    return cmdClear(a);
  if (c == "freeze")   return cmdFreeze();
  if (c == "pids")     return cmdPids();
  if (c == "vin")      return cmdVin();
  if (c == "ident")    return cmdIdent();
  if (c == "ktm")      return cmdKtm();
  if (c == "scan")     return cmdScan();
  if (c == "discover") return cmdDiscover();
  if (c == "monitor")  return cmdMonitor(a.length() ? constrain(a.toInt(), 500, 10000) : 2000);
  if (c == "sniff")    return cmdSniff(a);
  if (c == "modules")  return cmdModules();
  if (c == "mcodes")   return cmdMcodes(a);
  if (c == "mclear")   return cmdMclear(a);
  if (c == "peek")     return cmdPeek(a);
  if (c == "gear")     return cmdGear(a);
  if (c == "peak") {
    pkRpm = 0; pkSpd = 0; pkEct = -100;
    return "{\"t\":\"peak\",\"ok\":true,\"msg\":\"Peaks reset\"}";
  }
  if (c == "raw")      return cmdRaw(a);
  if (c == "settings") return cmdSettings();
  if (c == "set")      return cmdSet(a);
  if (c == "log")      return cmdLog(a);
  if (c == "logrows")  return cmdLogRows(a);
  if (c == "live") {
    bool on = a != "off";
    if (ch == CH_SERIAL) serialLive = on;
    else webLiveUntil = millis() + 3000;
    return liveJson();
  }
  if (c == "reboot") {
    rebootAt = millis() + 800;
    return "{\"t\":\"reboot\",\"ok\":true,\"msg\":\"Rebooting...\"}";
  }
  if (c == "factory") {
    if (a != "yes") return errJson("factory", "Send 'factory yes' to reset all settings");
    prefs.begin("obduke", false);
    prefs.clear();
    prefs.end();
    rebootAt = millis() + 800;
    return "{\"t\":\"factory\",\"ok\":true,\"msg\":\"Settings reset. Rebooting...\"}";
  }
  if (c == "help") {
    return "{\"t\":\"help\",\"ok\":true,\"msg\":\"status codes clear freeze live pids vin ident ktm scan "
           "discover monitor sniff modules mcodes mclear peek gear peak raw settings set log reboot factory\"}";
  }
  return errJson("error", "Unknown command: " + c);
}

// =====================================================================
//  Bluetooth Classic - ELM327 emulation for Torque Pro / the Teltonika tracker
// =====================================================================
// Speaks enough of the ELM327 command set for Torque Pro, Car Scanner and the Teltonika
// tracker. Requests are passed to the bike (engine ECU by default, or any module after
// AT SH). Multi-frame replies are formatted the way a real ELM327 does it.
struct ElmState {
  bool echo = true, lf = false, spaces = true, headers = false;
  uint32_t hdr = 0, cra = 0;   // 0 = automatic (engine ECU)
  uint16_t to = 300;           // reply timeout, ms
} elm;
static String elmBuf;

static void elmOut(const String &s) {
  SerialBT.print(s);
  SerialBT.print(elm.lf ? "\r\n" : "\r");
}
static void elmPrompt() { SerialBT.print(elm.lf ? "\r\n>" : "\r>"); }

static String elmHex(const uint8_t *d, int n) {
  String o;
  char b[4];
  for (int i = 0; i < n; i++) {
    snprintf(b, sizeof(b), "%02X", d[i]);
    if (i && elm.spaces) o += ' ';
    o += b;
  }
  return o;
}

static void elmReset() { elm = ElmState(); }

static String elmVolts() {
  float v;
  return (readVbat(v) ? fmtF(v, 1) : String("12.0")) + "V";
}

// Print a reply like an ELM327: single line, or "014 / 0: ... / 1: ..." for multi-frame
// (headers off), or the raw ISO-TP frames (headers on).
static void elmReply(uint32_t rid, const uint8_t *r, int n) {
  String sp = elm.spaces ? " " : "";
  String h = idStr(rid, false) + sp;
  if (n <= 7) {
    String line;
    if (elm.headers) { uint8_t pl = n; line = h + elmHex(&pl, 1) + sp; }
    elmOut(line + elmHex(r, n));
    return;
  }
  if (elm.headers) {
    uint8_t f[8] = {(uint8_t)(0x10 | ((n >> 8) & 0x0F)), (uint8_t)(n & 0xFF)};
    memcpy(f + 2, r, 6);
    elmOut(h + elmHex(f, 8));
    uint8_t seq = 1;
    for (int pos = 6; pos < n; pos += 7, seq++) {
      int k = min(7, n - pos);
      uint8_t c[8];
      c[0] = 0x20 | (seq & 0x0F);
      memcpy(c + 1, r + pos, k);
      elmOut(h + elmHex(c, k + 1));
    }
    return;
  }
  char l[12];
  snprintf(l, sizeof(l), "%03X", (unsigned)(n & 0xFFF));
  elmOut(l);
  elmOut("0:" + sp + elmHex(r, 6));
  int idx = 1;
  for (int pos = 6; pos < n; pos += 7, idx++) {
    char p[4];
    snprintf(p, sizeof(p), "%X:", idx & 0x0F);
    elmOut(String(p) + sp + elmHex(r + pos, min(7, n - pos)));
  }
}

static bool elmToEngine() { return !elm.hdr || elm.hdr == OBD_FUNC_ID || (ecuRespId && elm.hdr == ecuRespId - 8); }

// Send to wherever AT SH points. Returns length, sets rid to the replying ID.
static int elmRequest(const uint8_t *rq, int len, uint8_t *r, int maxLen, uint32_t &rid) {
  if (elmToEngine() && !elm.cra) {
    int n = obdRequest(rq, len, r, maxLen, elm.to);
    rid = ecuRespId ? ecuRespId : lastRespId;
    return n;
  }
  uint32_t tx = elm.hdr ? elm.hdr : (ecuRespId ? ecuRespId - 8 : 0x7E0);
  rid = elm.cra ? elm.cra : modRx(tx);
  return isoRequest(tx, rid, rq, len, r, maxLen, elm.to);
}

// One mode 01 PID, real or virtual. Returns data length (without 41 + PID), or -1.
static int elmPid01(uint8_t pid, uint8_t *d, uint32_t &rid) {
  bool virt = sBool(K_TRK_VIRT);
  float v;
  rid = ecuRespId ? ecuRespId : 0x7E8;
  if (virt && pid == 0x0C && !isSupp(0x0C) && ktmRpmOk && readKtmRpm(v, elm.to)) {
    uint16_t raw = (uint16_t)(v * 4);
    d[0] = raw >> 8; d[1] = raw & 0xFF;
    return 2;
  }
  if (virt && pid == 0x42 && !isSupp(0x42) && readVbat(v)) {
    uint16_t raw = (uint16_t)(v * 1000);
    d[0] = raw >> 8; d[1] = raw & 0xFF;
    return 2;
  }
  uint8_t rq[2] = {0x01, pid}, r[16];
  int n = elmRequest(rq, 2, r, sizeof(r), rid);
  if (n < 2 || r[0] != 0x41 || r[1] != pid) return -1;
  int dl = min(n - 2, 4);
  memcpy(d, r + 2, dl);
  if (virt && dl == 4) {                      // advertise the virtual PIDs
    if (pid == 0x00 && ktmRpmOk) d[1] |= 0x10;                // 0C
    if (pid == 0x40 && readVbat(v)) d[0] |= 0x40;             // 42
  }
  return dl;
}

// Virtual DIDs for Torque custom PIDs:  22 F0 01..04 = custom signals x10 (signed),
// 22 F0 10 = gear, 22 F0 11 = KTM RPM.  Returns reply length or 0.
static int elmVirtual22(const uint8_t *rq, int len, uint8_t *r) {
  if (!sBool(K_TRK_VIRT) || len != 3 || rq[0] != 0x22 || rq[1] != 0xF0) return 0;
  r[0] = 0x62; r[1] = 0xF0; r[2] = rq[2];
  if (rq[2] >= 0x01 && rq[2] <= 0x04) {
    const Sig &s = sigs[rq[2] - 1];
    if (!sigFresh(s)) return 0;
    int32_t x = lroundf(s.val * 10);
    x = constrain(x, -32768L, 32767L);
    r[3] = (uint16_t)x >> 8; r[4] = (uint16_t)x & 0xFF;
    return 5;
  }
  if (rq[2] == 0x10 && (live.ok & (1UL << L_GEAR)) && millis() - live.ms < 2000) {
    r[3] = (uint8_t)live.v[L_GEAR];
    return 4;
  }
  if (rq[2] == 0x11 && ktmRpmOk) {
    float v;
    if (!readKtmRpm(v, elm.to)) return 0;
    uint16_t raw = (uint16_t)v;
    r[3] = raw >> 8; r[4] = raw & 0xFF;
    return 5;
  }
  return 0;
}

static void elmAt(const String &a) {
  if (a == "Z" || a == "WS")       { elmReset(); elmOut(""); elmOut("ELM327 v1.5"); }
  else if (a == "D")               { bool e = elm.echo; elmReset(); elm.echo = e; elmOut("OK"); }
  else if (a == "I")               elmOut("ELM327 v1.5");
  else if (a == "@1" || a == "@2") elmOut("OBDuke");
  else if (a == "E0" || a == "E1") { elm.echo = a == "E1"; elmOut("OK"); }
  else if (a == "L0" || a == "L1") { elm.lf = a == "L1"; elmOut("OK"); }
  else if (a == "S0" || a == "S1") { elm.spaces = a == "S1"; elmOut("OK"); }
  else if (a == "H0" || a == "H1") { elm.headers = a == "H1"; elmOut("OK"); }
  else if (a == "DP")              elmOut(canRateIdx == 0 ? "AUTO, ISO 15765-4 (CAN 11/500)" : "AUTO, ISO 15765-4 (CAN 11/250)");
  else if (a == "DPN")             elmOut(canRateIdx == 0 ? "A6" : "A8");
  else if (a == "RV")              elmOut(elmVolts());
  else if (a == "AR")              { elm.hdr = 0; elm.cra = 0; elmOut("OK"); }
  else if (a.startsWith("SH")) {
    String h = a.substring(2);
    if (h.length() != 3) { elmOut("?"); return; }        // 29-bit headers not supported
    elm.hdr = strtoul(h.c_str(), nullptr, 16);
    if (elm.hdr == OBD_FUNC_ID) elm.hdr = 0;
    elmOut("OK");
  }
  else if (a.startsWith("CRA")) {
    String h = a.substring(3);
    elm.cra = h.length() == 3 ? strtoul(h.c_str(), nullptr, 16) : 0;
    elmOut("OK");
  }
  else if (a.startsWith("ST") && a.length() == 4) {
    uint32_t t = strtoul(a.substring(2).c_str(), nullptr, 16) * 4;
    elm.to = t ? constrain(t, 40UL, 1000UL) : 300;
    elmOut("OK");
  }
  else if (a == "MA" || a.startsWith("MR") || a.startsWith("MT") || a == "BD" || a.startsWith("PP")) elmOut("?");
  else elmOut("OK");   // SP, TP, AT, CAF, CFC, PC, M0, R1, etc: accepted, nothing to change
}

static void elmHandle(String cmd) {
  cmd.trim();
  String u;
  for (size_t i = 0; i < cmd.length(); i++) if (cmd[i] != ' ') u += (char)toupper(cmd[i]);
  if (elm.echo) elmOut(cmd);
  if (!u.length()) { elmPrompt(); return; }

  if (u.startsWith("AT")) { elmAt(u.substring(2)); elmPrompt(); return; }
  if (u.startsWith("ST")) { elmOut("?"); elmPrompt(); return; }      // STN chip commands

  for (size_t i = 0; i < u.length(); i++) if (!isxdigit(u[i])) { elmOut("?"); elmPrompt(); return; }
  uint8_t rq[8];
  int len = parseHex(u, rq, 7);   // an odd trailing digit (e.g. "010C1" reply count) is ignored
  if (!len) { elmOut("?"); elmPrompt(); return; }

  if (!ecuFound) { lastPing = 0; ecuTask(); }   // key just turned on: look for the ECU now

  uint8_t r[128];
  int n = 0;
  uint32_t rid = ecuRespId ? ecuRespId : 0x7E8;

  if (rq[0] == 0x01 && len >= 2 && elmToEngine() && !elm.cra) {
    // mode 01, possibly several PIDs at once (Torque "faster" mode): answer each, combine
    r[0] = 0x41;
    n = 1;
    for (int i = 1; i < len && i <= 6; i++) {
      uint8_t d[4];
      int dl = elmPid01(rq[i], d, rid);
      if (dl < 0) continue;
      r[n++] = rq[i];
      memcpy(r + n, d, dl);
      n += dl;
    }
    if (n == 1) n = 0;
  } else if (elmToEngine() && (n = elmVirtual22(rq, len, r)) > 0) {
    rid = ecuRespId ? ecuRespId : 0x7E8;
  } else {
    n = elmRequest(rq, len, r, sizeof(r), rid);
  }

  if (n <= 0) elmOut(!ecuFound && elmToEngine() && rq[0] == 0x01 && rq[1] == 0x00 ? "UNABLE TO CONNECT" : "NO DATA");
  else elmReply(rid, r, n);
  elmPrompt();
}

// A line starting with '!' is an OBDuke app command (same commands as the web page /api);
// the reply is one line of JSON. Anything else is ELM327 for Torque / the tracker.
static void btAppLine(const String &line) {
  btAppAt = millis();
  String r = runCommand(line.substring(1), CH_BT);
  SerialBT.print(r);
  SerialBT.print("\r\n");
}

static void trackerTask() {
  if (!trkStarted) return;
  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\r' || c == '\n') {
      if (elmBuf.length()) {
        if (elmBuf[0] == '!') btAppLine(elmBuf);
        else elmHandle(elmBuf);
        elmBuf = "";
      }
    } else if (elmBuf.length() < 200) {
      elmBuf += c;
    }
  }
}

// =====================================================================
//  Web server
// =====================================================================
static bool webAuth() {
  // no login unless both a user and a password are set
  if (!sStr(K_WEB_USER).length() || !sStr(K_WEB_PASS).length()) return true;
  if (web.authenticate(sStr(K_WEB_USER).c_str(), sStr(K_WEB_PASS).c_str())) return true;
  web.requestAuthentication(BASIC_AUTH, "OBDuke");
  return false;
}

static void webStart() {
  web.on("/", HTTP_GET, []() {
    if (!webAuth()) return;
    web.send_P(200, "text/html", WEB_UI);
  });
  web.on("/api", []() {
    if (!webAuth()) return;
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", runCommand(web.arg("c"), CH_WEB));
  });
  web.on("/live", HTTP_GET, []() {
    if (!webAuth()) return;
    webLiveUntil = millis() + 3000;
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", liveJson());
  });
  web.on("/log.csv", HTTP_GET, []() {
    if (!webAuth()) return;
    web.sendHeader("Content-Disposition", "attachment; filename=obduke_log.csv");
    web.setContentLength(CONTENT_LENGTH_UNKNOWN);
    web.send(200, "text/csv", "");
    web.sendContent(logHeader());
    String chunk;
    for (int i = 0; i < logCount; i++) {
      chunk += logRow(logIndex(i));
      if (chunk.length() > 1000) { web.sendContent(chunk); chunk = ""; }
    }
    if (chunk.length()) web.sendContent(chunk);
    web.sendContent("");
  });
  web.on("/manifest.json", HTTP_GET, []() {
    web.send(200, "application/manifest+json",
             "{\"name\":\"OBDuke\",\"short_name\":\"OBDuke\",\"start_url\":\"/\",\"display\":\"standalone\","
             "\"background_color\":\"#0a0a0a\",\"theme_color\":\"#ff6600\","
             "\"icons\":[{\"src\":\"/icon.svg\",\"sizes\":\"any\",\"type\":\"image/svg+xml\"}]}");
  });
  web.on("/icon.svg", HTTP_GET, []() { web.send_P(200, "image/svg+xml", ICON_SVG); });

  // Captive portal probes from Android / iOS / Windows
  auto toRoot = []() {
    web.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    web.send(302, "text/plain", "");
  };
  web.on("/generate_204", toRoot);
  web.on("/gen_204", toRoot);
  web.on("/hotspot-detect.html", toRoot);       // iPhone / Mac
  web.on("/library/test/success.html", toRoot);
  web.on("/canonical.html", toRoot);            // Firefox
  web.on("/success.txt", toRoot);
  web.on("/mobile/status.php", toRoot);         // older Android
  web.on("/redirect", toRoot);
  web.on("/fwlink", toRoot);                    // Windows
  web.on("/connecttest.txt", toRoot);
  web.on("/ncsi.txt", toRoot);
  web.onNotFound(toRoot);

  web.begin();
  webStarted = true;
}

static void wifiStart() {
  int mode = sInt(K_WIFI_MODE);
  if (mode == 2) { WiFi.mode(WIFI_OFF); return; }
  WiFi.setHostname(sStr(K_HOSTNAME).c_str());
  bool sta = mode == 1 && sStr(K_STA_SSID).length();
  WiFi.mode(sta ? WIFI_AP_STA : WIFI_AP);
  const char *pass = sStr(K_AP_PASS).length() >= 8 ? sStr(K_AP_PASS).c_str() : nullptr;
  WiFi.softAP(sStr(K_AP_SSID).c_str(), pass, sInt(K_AP_CH), sBool(K_AP_HIDDEN));
  // Hotspot address (default 1.1.1.1). Phones get 1.1.1.2 onwards; DNS points back at us
  // so the captive portal still opens the page.
  IPAddress ip;
  if (!ip.fromString(sStr(K_AP_IP))) ip = IPAddress(1, 1, 1, 1);
  delay(100);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  if (!WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0), IPAddress((uint32_t)0), ip))
#else
  if (!WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0)))
#endif
    Serial.println("Hotspot address not accepted, using default");
  if (sta) WiFi.begin(sStr(K_STA_SSID).c_str(), sStr(K_STA_PASS).c_str());
  if (sBool(K_CAPTIVE)) {
    dns.setErrorReplyCode(DNSReplyCode::NoError);
    dns.start(53, "*", WiFi.softAPIP());
    dnsStarted = true;
  }
  MDNS.begin(sStr(K_HOSTNAME).c_str());
  MDNS.addService("http", "tcp", 80);
  webStart();
  Serial.printf("Hotspot %s at %s\n", sStr(K_AP_SSID).c_str(), WiFi.softAPIP().toString().c_str());
}

// =====================================================================
//  Status LED: slow blink = no ECU, solid = ECU ok, fast blink = FI light on
// =====================================================================
static void ledTask() {
  int pin = sInt(K_LED_PIN);
  if (pin <= 0) return;
  uint32_t t = millis();
  bool on;
  if (!ecuFound) on = (t / 700) % 2;
  else if (live.fi == 1) on = (t / 150) % 2;
  else on = true;
  digitalWrite(pin, on);
}

// =====================================================================
//  Setup / loop
// =====================================================================
static String serialBuf;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nOBDuke " FW_VERSION);
  loadSettings();
  loadGears();
  for (int i = 0; i < 4; i++) {
    String e = sigParse(sStr(K_SIG1 + i), sigs[i]);
    if (e.length()) Serial.printf("Signal %d ignored: %s\n", i + 1, e.c_str());
  }
  if (sInt(K_LED_PIN) > 0) pinMode(sInt(K_LED_PIN), OUTPUT);

  // CAN: find the rate (listen-only, never disturbs the bus), then go normal
  int rateSel = sInt(K_CAN_RATE);
  canRateIdx = rateSel == 2 ? 1 : 0;
  if (rateSel == 0) {
    Serial.println("Detecting CAN rate...");
    if (probeRate(0)) canRateIdx = 0;
    else if (probeRate(1)) canRateIdx = 1;
    else canRateIdx = 0;   // quiet bus: Duke default 500k
  }
  if (!installTwai(TWAI_MODE_NORMAL, canRateIdx)) Serial.println("CAN driver failed - check pins");

  // Start Wi-Fi FIRST while memory is plentiful - it needs large buffers.
  // Bluetooth Classic (tracker) starts after and uses what is left.
  Serial.printf("Free heap %lu\n", (unsigned long)ESP.getFreeHeap());
  Serial.println("Starting Wi-Fi...");
  wifiStart();
  Serial.printf("Wi-Fi up. Free heap %lu\n", (unsigned long)ESP.getFreeHeap());

  if (sBool(K_TRK_ON)) {
    // We only use Bluetooth Classic - hand the BLE controller's RAM back to the heap
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    String pin = sStr(K_TRK_PIN);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    SerialBT.setPin(pin.c_str(), pin.length());
#else
    SerialBT.setPin(pin.c_str());
#endif
    trkStarted = SerialBT.begin(sStr(K_BT_NAME));
    Serial.printf("Bluetooth ELM327 %s. Free heap %lu\n", trkStarted ? "ready" : "FAILED", (unsigned long)ESP.getFreeHeap());
  }
  if (sBool(K_LOG_AUTO)) { logging = true; logStartMs = millis(); }
  Serial.println("Ready. Type 'help'.");
}

void loop() {
  // USB serial commands
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuf.length()) { Serial.println(runCommand(serialBuf, CH_SERIAL)); serialBuf = ""; }
    } else if (serialBuf.length() < 160) serialBuf += c;
  }


  trackerTask();
  if (webStarted) web.handleClient();
  if (dnsStarted) dns.processNextRequest();

  ecuTask();

  bool wantLive = serialLive || logging || (int32_t)(webLiveUntil - millis()) > 0;
  if (wantLive && millis() - lastLivePoll >= (uint32_t)sInt(K_LIVE_MS)) {
    lastLivePoll = millis();
    pollLive();
    String j = liveJson();
    if (serialLive) Serial.println(j);
  }
  if (logging && millis() - lastLogMs >= (uint32_t)sInt(K_LOG_MS)) {
    lastLogMs = millis();
    logSample();
  }

  twai_message_t m;
  while (twai_receive(&m, 0) == ESP_OK) trackFrame(m);
  checkBusHealth();
  ledTask();

  if (rebootAt && (int32_t)(millis() - rebootAt) > 0) ESP.restart();
  delay(1);
}
