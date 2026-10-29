#include "sim.cpp"
#include "../firmware/obduke/obduke_fw.cpp"
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("  FAIL: %s\n", m); fails++; } else printf("  ok: %s\n", m); } while (0)
static bool has(const std::string &s, const char *x) { return s.find(x) != std::string::npos; }
static void spin(uint32_t ms) { uint32_t t = g_now + ms; while (g_now < t) loop(); }
static std::string BT(const char *c) { SerialBT.out.clear(); SerialBT.in = std::string(c) + "\r"; SerialBT.pos = 0; loop(); return SerialBT.out; }
int main() {
  setup(); spin(4000);
  std::string r = BT("!status");
  CHECK(r.rfind("{\"t\":\"status\"", 0) == 0 && r.size() > 10 && r.substr(r.size() - 2) == "\r\n", "!status -> one JSON line, no echo/prompt");
  CHECK(has(r, "\"fw\":\"1.2.1\""), "fw 1.2.1");
  r = BT("!status"); CHECK(has(r, "\"bt_mode\":\"app\""), "bt_mode app");
  r = BT("!live"); spin(600); r = BT("!live");
  CHECK(has(r, "\"t\":\"live\"") && has(r, "\"rpm\":1500"), "!live gives live data");
  r = BT("!set sta_pass abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0");
  CHECK(has(r, "\"ok\":true"), "long command (>64 chars) over BT");
  r = BT("!log start"); bike.rpm = 4000; bike.rear = 40;
  for (int i = 0; i < 25; i++) { BT("!live"); spin(200); }
  r = BT("!logrows 0 2"); puts(r.c_str());
  CHECK(has(r, "\"header\":\"seconds,rpm") && has(r, "\"n\":2"), "logrows first chunk has header");
  r = BT("!logrows 2 80"); CHECK(!has(r, "\"header\"") && has(r, "\"start\":2"), "logrows next chunk");
  // ELM still works on the same link
  r = BT("ATE0"); CHECK(has(r, "OK"), "ELM ATE0 still works");
  r = BT("010C"); CHECK(has(r, "41 0C 3E 80"), "ELM 010C still works");
  r = BT("!modules"); CHECK(has(r, "\"t\":\"modules\"") && has(r, "7B0"), "!modules over BT");
  r = BT("!sniff start"); spin(800); r = BT("!sniff"); CHECK(has(r, "\"id\":\"120\""), "!sniff over BT");
  r = BT("!bogus"); CHECK(has(r, "Unknown command"), "unknown app command -> JSON error");
  printf("%d failure(s)\n", fails);
}
