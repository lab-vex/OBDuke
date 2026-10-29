#include "sim.cpp"
#include "../firmware/obduke/obduke_fw.cpp"
#include <cassert>
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("  FAIL: %s\n", msg); fails++; } else printf("  ok: %s\n", msg); } while (0)
static bool has(const String &s, const char *x) { return s.s.find(x) != std::string::npos; }
static String run(const char *c) { return runCommand(c, CH_WEB); }
static void spin(uint32_t ms) { uint32_t t = g_now + ms; while (g_now < t) { loop(); } }
static String E(const char *c) {
  SerialBT.out.clear(); SerialBT.in = std::string(c) + "\r"; SerialBT.pos = 0;
  loop();
  std::string o = SerialBT.out;
  for (auto &ch : o) if (ch == '\r') ch = '|';
  return String(o);
}
int main() {
  puts("== boot"); setup();
  CHECK(WiFi.softAPIP().toString() == "1.1.1.1", "hotspot address 1.1.1.1");
  spin(4000);
  String st = run("status");
  CHECK(has(st, "\"ecu\":true"), "ECU found");
  CHECK(has(st, "\"ap_ip\":\"1.1.1.1\""), "status shows 1.1.1.1");

  puts("== live, gear, peaks");
  bike.rpm = 6000; bike.rear = 60; bike.front = 60;   // 100 rpm per km/h = 3rd gear
  run("live"); spin(1500);
  String lv = liveJson(); puts(lv.c_str());
  CHECK(has(lv, "\"rpm\":6000"), "RPM via KTM 2121");
  CHECK(has(lv, "\"spd\":60"), "speed");
  CHECK(has(lv, "\"gear\":3"), "gear 3 calculated");
  CHECK(has(lv, "\"pk\":{\"rpm\":6000"), "peak rpm");
  bike.rpm = 7000; bike.rear = 100;  // 70/kmh = 5th
  run("live"); spin(800);
  CHECK(has(liveJson(), "\"gear\":5"), "gear 5");
  bike.rpm = 9000; bike.rear = 60;   // 150 = between 1st and 2nd -> clutch slipping
  run("live"); spin(800);
  CHECK(has(liveJson(), "\"gear\":0"), "no gear when ratio matches nothing");
  String g = run("gear learn 2"); puts(g.c_str());
  CHECK(has(g, "150.0"), "gear learn stores 150 rpm/kmh for 2nd");
  run("live"); spin(800);
  CHECK(has(liveJson(), "\"gear\":2"), "learned 2nd now detected");
  run("gear reset");
  CHECK(has(run("peak reset"), "Peaks reset"), "peak reset");

  puts("== custom signals");
  String e = run("set sig1 120:0:2:b:0.01:0:Front wheel:km/h"); puts(e.c_str());
  CHECK(has(e, "\"ok\":true"), "sig1 accepted");
  CHECK(has(run("set sig2 2122:3:1:b:0.1:0:Battery:V"), "\"ok\":true"), "sig2 (KTM local ID) accepted");
  CHECK(has(run("set sig3 120:7:2:b:1:0:x:y"), "past the end"), "sig3 bad byte rejected");
  CHECK(has(run("set sig3 hello"), "Format"), "sig3 bad format rejected");
  bike.front = 42.5;
  run("live"); spin(800);
  lv = liveJson(); puts(lv.c_str());
  CHECK(has(lv, "\"c1\":42.50"), "front wheel from broadcast 0x120");
  CHECK(has(lv, "\"c2\":13.80"), "battery from local ID 2122 byte 3");
  CHECK(has(run("status"), "\"n\":\"Front wheel\""), "status lists signal names");
  CHECK(has(logHeader(), "Front wheel_km/h"), "CSV header has signal name");

  puts("== sniffer");
  run("sniff start"); spin(1000);
  String sn = run("sniff"); printf("%.300s\n", sn.c_str());
  CHECK(has(sn, "\"id\":\"120\""), "sniffer sees 0x120");
  CHECK(has(sn, "\"id\":\"280\""), "sniffer sees 0x280");
  run("sniff mark"); bike.front = 80; spin(500);
  sn = run("sniff");
  { auto p = sn.s.find("\"id\":\"120\""); auto seg = sn.s.substr(p, 200); puts(seg.c_str());
    CHECK(seg.find("\"m\":19") != std::string::npos || seg.find("\"m\":3") != std::string::npos || seg.find("\"m\":17") != std::string::npos, "bytes 0-1 (front wheel) flagged changed since mark"); }
  { auto p = sn.s.find("\"id\":\"280\""); auto seg = sn.s.substr(p, 120); CHECK(seg.find("\"m\":0") != std::string::npos, "0x280 unchanged since mark"); }
  run("sniff stop");

  puts("== modules / ABS codes");
  String md = run("modules"); puts(md.c_str());
  CHECK(has(md, "\"tx\":\"7B0\",\"rx\":\"7B8\""), "ABS found at 7B0/7B8");
  CHECK(has(md, "\"tx\":\"7E0\",\"rx\":\"7E8\""), "engine ECU found");
  CHECK(has(md, "BOSCH ABS 9M+"), "ABS identity read (multi-frame)");
  CHECK(!has(md, "\"tx\":\"710\""), "broadcast ID 710 skipped");
  String mc = run("mcodes 7B0"); puts(mc.c_str());
  CHECK(has(mc, "C0031") && has(mc, "C2121"), "ABS codes C0031 + C2121 (KWP 18, multi-frame)");
  CHECK(has(run("mclear 7B0"), "yes"), "mclear needs confirm");
  CHECK(has(run("mclear 7B0 yes"), "cleared (KWP2000)"), "ABS clear");
  mc = run("mcodes 7E0"); puts(mc.c_str());
  CHECK(has(mc, "P0480"), "engine codes via module path (OBD fallback)");

  puts("== watch (peek)");
  String pk = run("peek 21 22"); puts(pk.c_str());
  CHECK(has(pk, "61 22 01 02 03 8A"), "peek KTM local ID multi-frame");
  CHECK(has(run("peek @7B0 21 05"), "61 05 03 20"), "peek ABS local ID");
  CHECK(has(run("peek 31 01 00"), "only allows read"), "peek refuses routine control");
  CHECK(has(run("peek 3E"), "only allows read"), "peek refuses 3E");

  puts("== ELM327 / Torque sequence");
  String r;
  r = E("ATZ"); puts(r.c_str()); CHECK(has(r, "ELM327 v1.5|"), "ATZ");
  r = E("ATE0"); CHECK(has(r, "OK|"), "ATE0");
  r = E("ATL0"); r = E("ATS0"); r = E("ATH0"); r = E("ATSP0"); r = E("ATAT1");
  r = E("0100"); puts(r.c_str()); CHECK(r == "4100BE1FB811||>" || has(r, "4100"), "0100 reply");
  { // PID 0C must now be advertised
    std::string s = r.s.substr(r.s.find("4100") + 4, 8); uint32_t bm = strtoul(s.c_str(), 0, 16);
    CHECK(bm & (1u << (32 - 0x0C)), "0100 advertises virtual 0C");
  }
  r = E("010C"); puts(r.c_str()); CHECK(has(r, "410C8CA0"), "010C virtual RPM 9000");
  r = E("010D1"); CHECK(has(r, "410D3C"), "010D with reply count digit");
  r = E("010C0D05"); puts(r.c_str()); CHECK(has(r, "008|0:410C8CA00D3C|1:057D"), "multi-PID request combined");
  r = E("ATDPN"); CHECK(has(r, "A6"), "ATDPN");
  r = E("ATRV"); puts(r.c_str()); CHECK(has(r, "V|"), "ATRV");
  r = E("0902"); puts(r.c_str()); CHECK(has(r, "014|0:490201564B4A|1:474A3430304443|2:32313631373400") || has(r,"014|0:"), "VIN multi-frame (headers off)");
  r = E("ATS1"); r = E("ATH1");
  r = E("010D"); puts(r.c_str()); CHECK(has(r, "7E8 03 41 0D 3C"), "headers on");
  r = E("0902"); puts(r.c_str()); CHECK(has(r, "7E8 10 14 49 02 01"), "VIN multi-frame (headers on)");
  r = E("ATH0");
  r = E("ATSH7B0"); CHECK(has(r, "OK"), "ATSH 7B0");
  r = E("1A9B"); puts(r.c_str()); CHECK(has(r, "00F|0: 5A 9B 42 4F 53 43"), "ABS ident via ATSH (multi-frame)");
  r = E("2105"); puts(r.c_str()); CHECK(has(r, "61 05 03 20"), "ABS local ID via ATSH");
  r = E("ATSH7E0"); r = E("2121"); puts(r.c_str()); CHECK(has(r, "61 21 8C A0"), "KTM 2121 pass-through for Torque custom PID");
  r = E("22F001"); puts(r.c_str()); CHECK(has(r, "62 F0 01 03 20"), "virtual DID F001 = front wheel x10 (80.0)");
  r = E("22F011"); CHECK(has(r, "62 F0 01") == false && has(r, "62 F0 11 23 28"), "virtual DID F011 = RPM 9000");
  r = E("STI"); CHECK(has(r, "?"), "STI -> ?");
  r = E("ATMA"); CHECK(has(r, "?"), "ATMA -> ?");
  r = E("HELLO"); CHECK(has(r, "?"), "junk -> ?");
  r = E("0112"); puts(r.c_str()); CHECK(has(r, "NO DATA"), "unsupported PID -> NO DATA");

  puts("== settings validation");
  CHECK(has(run("set ap_ip 10.1.1.1"), "\"ok\":true"), "ap_ip 10.1.1.1 accepted");
  CHECK(has(run("set ap_ip 1.1.1.300"), "normal IP"), "bad ap_ip rejected");
  CHECK(has(run("set ap_ip 127.0.0.1"), "normal IP"), "loopback rejected");
  CHECK(has(run("set gears 1,2,3"), "6 numbers"), "gears needs 6");
  CHECK(has(run("monitor 1000"), "\"id\":\"120\""), "old 3s monitor still works");
  printf("\n%d failure(s)\n", fails);
  return fails ? 1 : 0;
}
