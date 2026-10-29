// Simulated Gen1 Duke CAN bus: engine ECU (7E0/7E8), ABS (7B0/7B8), broadcast frames.
#include "mock/Arduino.h"
#include "mock/driver/twai.h"
#include <deque>
#include <vector>
#include <map>
uint32_t g_now = 1000;
SerialMock Serial; EspMock ESP;
#include "mock/WiFi.h"
#include "mock/ESPmDNS.h"
WiFiMock WiFi; MDNSMock MDNS;
std::map<std::string, std::string> g_nvs;

struct Bike { float rpm = 1500, front = 0, rear = 0, ect = 85, tps = 3; } bike;
static std::deque<twai_message_t> rxq;
struct Pending { uint32_t rid; std::vector<uint8_t> data; size_t pos; uint8_t seq; };
static std::map<uint32_t, Pending> pend;   // keyed by module tx id
static uint32_t nextB120 = 0, nextB280 = 0, nextB7xx = 0;
int g_txCount = 0; bool g_ecuOff = false;

static void q(uint32_t id, std::vector<uint8_t> d) {
  twai_message_t m = {}; m.identifier = id; m.data_length_code = 8;
  for (size_t i = 0; i < 8; i++) m.data[i] = i < d.size() ? d[i] : 0x55;
  rxq.push_back(m);
}
static void reply(uint32_t tx, uint32_t rid, std::vector<uint8_t> p) {
  if (p.size() <= 7) { std::vector<uint8_t> f = {(uint8_t)p.size()}; f.insert(f.end(), p.begin(), p.end()); q(rid, f); return; }
  std::vector<uint8_t> f = {(uint8_t)(0x10 | (p.size() >> 8)), (uint8_t)(p.size() & 0xFF)};
  f.insert(f.end(), p.begin(), p.begin() + 6);
  q(rid, f);
  pend[tx] = {rid, p, 6, 1};
}
static void bcast(uint32_t until) {
  while (nextB120 <= until) {
    uint16_t f = bike.front * 100, r = bike.rear * 100; static uint8_t ctr = 0;
    q(0x120, {(uint8_t)(f >> 8), (uint8_t)f, (uint8_t)(r >> 8), (uint8_t)r, ctr++, 0, 0, 0});
    nextB120 += 20;
  }
  while (nextB280 <= until) { q(0x280, {(uint8_t)(bike.ect + 40), 0x12, 0x34, 0, 0, 0, 0, 0}); nextB280 += 100; }
  while (nextB7xx <= until) { q(0x710, {1, 2, 3, 4, 5, 6, 7, 8}); nextB7xx += 200; }  // a broadcast that lives in the 7xx range
}
void sim_advance(uint32_t ms) { g_now += ms; bcast(g_now); }

static std::vector<uint8_t> bitmap(uint8_t base, std::vector<uint8_t> pids) {
  uint8_t b[4] = {0};
  for (uint8_t p : pids) if (p > base && p <= base + 32) { int i = p - base - 1; b[i / 8] |= 0x80 >> (i % 8); }
  return {0x41, base, b[0], b[1], b[2], b[3]};
}
static const std::vector<uint8_t> SUP = {0x01, 0x05, 0x06, 0x07, 0x0B, 0x0D, 0x0E, 0x0F, 0x11, 0x13, 0x14, 0x1C, 0x20, 0x21, 0x30, 0x31, 0x40, 0x4F, 0x50};

static void ecu(uint32_t tx, const uint8_t *r, int n) {
  uint32_t rid = 0x7E8;
  if (r[0] == 0x01 && n >= 2) {
    uint8_t p = r[1];
    if (p == 0x00 || p == 0x20 || p == 0x40) { reply(tx, rid, bitmap(p, SUP)); return; }
    if (p == 0x01) { reply(tx, rid, {0x41, 0x01, 0x81, 0x07, 0, 0}); return; }
    if (p == 0x05) { reply(tx, rid, {0x41, 0x05, (uint8_t)(bike.ect + 40)}); return; }
    if (p == 0x0D) { reply(tx, rid, {0x41, 0x0D, (uint8_t)bike.rear}); return; }
    if (p == 0x11) { reply(tx, rid, {0x41, 0x11, (uint8_t)(bike.tps * 255 / 100)}); return; }
    if (p == 0x0B) { reply(tx, rid, {0x41, 0x0B, 35}); return; }
    if (p == 0x0F) { reply(tx, rid, {0x41, 0x0F, 70}); return; }
    if (p == 0x0E) { reply(tx, rid, {0x41, 0x0E, 140}); return; }
    if (p == 0x06 || p == 0x07) { reply(tx, rid, {0x41, p, 130}); return; }
    if (p == 0x14) { reply(tx, rid, {0x41, 0x14, 90, 0xFF}); return; }
    if (p == 0x21 || p == 0x31) { reply(tx, rid, {0x41, p, 0, 12}); return; }
    if (p == 0x30) { reply(tx, rid, {0x41, 0x30, 9}); return; }
    reply(tx, rid, {0x7F, 0x01, 0x12}); return;
  }
  if (r[0] == 0x21 && n >= 2 && r[1] == 0x21) { uint16_t v = bike.rpm * 4; reply(tx, rid, {0x61, 0x21, (uint8_t)(v >> 8), (uint8_t)v}); return; }
  if (r[0] == 0x21 && n >= 2 && r[1] == 0x22) { reply(tx, rid, {0x61, 0x22, 1, 2, 3, 138, 5, 6, 7, 8, 9, 10}); return; }
  if (r[0] == 0x09 && r[1] == 0x02) {
    std::vector<uint8_t> v = {0x49, 0x02, 0x01}; const char *vin = "VBKJGJ400DC216174"; v.insert(v.end(), vin, vin + 17);
    reply(tx, rid, v); return;
  }
  if (r[0] == 0x03) { reply(tx, rid, {0x43, 0x01, 0x04, 0x80}); return; }
  if (r[0] == 0x07) { reply(tx, rid, {0x47, 0x00}); return; }
  if (r[0] == 0x04) { reply(tx, rid, {0x44}); return; }
  if (r[0] == 0x3E) { reply(tx, rid, {0x7E, 0x00}); return; }
  reply(tx, rid, {0x7F, r[0], 0x11});
}
static void abs_(uint32_t tx, const uint8_t *r, int n) {
  uint32_t rid = 0x7B8;
  if (r[0] == 0x3E) { reply(tx, rid, {0x7E, 0x00}); return; }
  if (r[0] == 0x18) { reply(tx, rid, {0x58, 0x02, 0x40, 0x31, 0xE0, 0x61, 0x21, 0x60}); return; }
  if (r[0] == 0x14) { reply(tx, rid, {0x54, 0xFF, 0x00}); return; }
  if (r[0] == 0x1A && r[1] == 0x9B) { std::vector<uint8_t> v = {0x5A, 0x9B}; const char *s = "BOSCH ABS 9M+"; v.insert(v.end(), s, s + 13); reply(tx, rid, v); return; }
  if (r[0] == 0x21 && r[1] == 0x05) { uint16_t f = bike.front * 10; reply(tx, rid, {0x61, 0x05, (uint8_t)(f >> 8), (uint8_t)f}); return; }
  reply(tx, rid, {0x7F, r[0], 0x11});
}

esp_err_t twai_driver_install(const twai_general_config_t *, const twai_timing_config_t *, const twai_filter_config_t *) { return ESP_OK; }
esp_err_t twai_driver_uninstall() { return ESP_OK; }
esp_err_t twai_start() { return ESP_OK; }
esp_err_t twai_stop() { return ESP_OK; }
esp_err_t twai_get_status_info(twai_status_info_t *s) { s->state = TWAI_STATE_RUNNING; return ESP_OK; }
esp_err_t twai_initiate_recovery() { return ESP_OK; }
esp_err_t twai_transmit(const twai_message_t *m, uint32_t) {
  g_txCount++;
  uint32_t id = m->identifier;
  const uint8_t *d = m->data;
  if ((d[0] >> 4) == 3) {                      // flow control from the tester
    auto it = pend.find(id);
    if (it == pend.end() && id == 0x7DF) it = pend.find(0x7E0);
    if (it == pend.end()) return ESP_OK;
    Pending &p = it->second;
    while (p.pos < p.data.size()) {
      std::vector<uint8_t> f = {(uint8_t)(0x20 | (p.seq++ & 0x0F))};
      size_t k = std::min<size_t>(7, p.data.size() - p.pos);
      f.insert(f.end(), p.data.begin() + p.pos, p.data.begin() + p.pos + k);
      p.pos += k;
      q(p.rid, f);
    }
    pend.erase(it);
    return ESP_OK;
  }
  if ((d[0] >> 4) != 0) return ESP_OK;
  int n = d[0] & 0x0F;
  if ((id == 0x7DF || id == 0x7E0) && !g_ecuOff) { ecu(0x7E0, d + 1, n); if (id == 0x7DF && pend.count(0x7E0)) {} }
  else if (id == 0x7B0) abs_(0x7B0, d + 1, n);
  return ESP_OK;
}
esp_err_t twai_receive(twai_message_t *m, uint32_t ticks) {
  bcast(g_now);
  if (rxq.empty() && ticks) sim_advance(ticks);
  if (rxq.empty()) return ESP_FAIL;
  *m = rxq.front(); rxq.pop_front();
  return ESP_OK;
}
void sim_clear() { rxq.clear(); }
