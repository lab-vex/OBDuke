package au.obduke.app

import kotlinx.coroutines.delay
import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt
import kotlin.math.sin

/**
 * A pretend OBDuke unit for trying the app without hardware. It answers the same commands with
 * the same JSON as firmware 1.2, and plays a looping 45 s ride: idle, a run up through the gears,
 * a cruise, then slowing back down. Some codes are stored so the Codes screen has something on it,
 * and settings changes are kept until you leave demo mode.
 */
class DemoLink : ObdLink {

    private var t0 = System.currentTimeMillis()
    private var connected = false

    // ---------------- pretend unit state ----------------
    private val settings = LinkedHashMap<String, String>()
    private var stored = mutableListOf("P0130")
    private var pending = mutableListOf("P0505")
    private var absCodes = mutableListOf("C0035")
    private var pkRpm = 0.0
    private var pkSpd = 0.0
    private var pkEct = -1000.0
    private var logOn = false
    private var logStart = 0L
    private var logFrozenN = 0
    private var sniffOn = false
    private var sniffStart = 0L
    private val sniffMark = HashMap<String, IntArray>()   // id -> [lo bytes..., hi bytes...]
    private val sniffPrev = HashMap<String, List<Int>>()

    init { factoryReset() }

    private fun factoryReset() {
        settings.clear()
        for (d in DEFS) settings[d.key] = d.def
        // two custom signals, so the dashboard shows what they look like
        settings["sig1"] = "120:2:2:b:0.01:0:Front wheel:km/h"
        settings["sig2"] = "120:4:2:b:0.01:0:Rear wheel:km/h"
        stored = mutableListOf("P0130"); pending = mutableListOf("P0505"); absCodes = mutableListOf("C0035")
    }

    fun connect(): String {
        connected = true
        t0 = System.currentTimeMillis()
        return statusJson().toString()
    }

    override fun disconnect() { connected = false; sniffOn = false }

    private fun now() = (System.currentTimeMillis() - t0) / 1000.0

    // ---------------- the ride ----------------
    private fun gears(): List<Double> {
        val g = (settings["gears"] ?: "").split(",").mapNotNull { it.trim().toDoubleOrNull() }
        return if (g.size == 6) g else listOf(175.0, 131.0, 99.0, 82.0, 73.0, 67.0)
    }

    private fun calcGear(rpm: Double, spd: Double): Int {
        if (spd < 8 || rpm < 1500) return 0
        val r = rpm / spd
        var best = 0
        var bestErr = 1e9
        gears().forEachIndexed { i, g ->
            if (g > 0) { val e = abs(r - g) / g; if (e < bestErr) { bestErr = e; best = i + 1 } }
        }
        return if (bestErr < 0.12) best else 0
    }

    /** Engine values at [t] seconds since connecting. */
    private fun sample(t: Double): Map<String, Double> {
        val g = gears()
        val p = t % 45.0
        val jit = sin(t * 7.3) * 25 + sin(t * 3.1) * 15
        var rpm: Double
        var spd: Double
        var tps: Double
        when {
            p < 6 -> { rpm = 1450 + jit; spd = 0.0; tps = 0.0 }
            p < 24 -> {                                    // 3 s in each gear
                val gi = ((p - 6) / 3).toInt().coerceIn(0, 5)
                val f = ((p - 6) % 3) / 3
                // shift at 9200 rpm; the revs drop by the ratio change. 6th eases off into the cruise.
                val top = if (gi < 5) 9200.0 else 6700.0
                val bottom = if (gi == 0) 3500.0 else 9200.0 * g[gi] / g[gi - 1]
                rpm = bottom + (top - bottom) * f
                spd = rpm / g[gi]
                tps = if (gi < 5) 92 + 8 * f else 35.0
            }
            p < 34 -> { rpm = 6700 + jit; spd = rpm / g[5]; tps = 28 + sin(t) * 3 }
            else -> {                                      // braking, changing down
                val f = (p - 34) / 11
                spd = 100 * (1 - f)
                if (spd < 10) { rpm = 1450 + jit; tps = 0.0 }
                else {
                    val gi = (5 downTo 0).firstOrNull { spd * g[it] >= 3000 } ?: 0
                    rpm = spd * g[gi]; tps = 0.0
                }
            }
        }
        val warm = min(1.0, t / 300.0)                     // 5 min warm-up
        val ect = 45 + 47 * warm + sin(t / 9) * 1.5
        val closedLoop = tps < 70
        return mapOf(
            "rpm" to rpm, "tps" to tps, "spd" to spd,
            "map" to if (tps == 0.0 && spd > 10) 24.0 else 32 + tps * 0.68,
            "ect" to ect, "iat" to 31 + sin(t / 20) * 2,
            "adv" to if (rpm < 2000) 12.0 else 34 - tps * 0.12,
            "stft" to if (closedLoop) sin(t * 1.7) * 4.2 else 0.0,
            "ltft" to 2.3,
            "o2" to if (closedLoop) 0.47 + 0.38 * sin(t * 2 * PI * 0.9) else 0.86,
            "vbat" to if (rpm > 2500) 13.92 + sin(t) * 0.04 else 13.21,
            "gear" to calcGear(rpm, spd).toDouble(),
            "c1" to spd, "c2" to spd * 1.02,
            "c3" to Double.NaN, "c4" to Double.NaN
        )
    }

    private fun sigOn(i: Int) = (settings["sig$i"] ?: "").split(":").size >= 6

    private fun round(v: Double, dp: Int): Double {
        var m = 1.0; repeat(dp) { m *= 10 }
        return (v * m).roundToInt() / m
    }

    private fun liveJson(): JSONObject {
        val s = sample(now())
        pkRpm = max(pkRpm, s["rpm"]!!); pkSpd = max(pkSpd, s["spd"]!!); pkEct = max(pkEct, s["ect"]!!)
        val o = JSONObject().put("t", "live").put("ms", (now() * 1000).toLong()).put("ecu", true)
        for ((k, dp) in LIVE_DP) {
            val v = s[k] ?: Double.NaN
            val on = !v.isNaN() && (!k.startsWith("c") || sigOn(k.substring(1).toInt()))
            o.put(k, if (on) round(v, dp) else JSONObject.NULL)
        }
        o.put("fi", if (stored.isEmpty()) 0 else 1).put("dtc", stored.size)
        o.put("pk", JSONObject().put("rpm", pkRpm.toInt()).put("spd", pkSpd.toInt()).put("ect", pkEct.toInt()))
        o.put("log", logOn)
        return o
    }

    // ---------------- commands ----------------
    override suspend fun live(): String {
        check()
        delay(30)
        return liveJson().toString()
    }

    override suspend fun cmd(c: String, timeoutMs: Int): String {
        check()
        val line = c.trim()
        val sp = line.indexOf(' ')
        val name = if (sp < 0) line else line.substring(0, sp)
        val a = if (sp < 0) "" else line.substring(sp + 1).trim()
        delay(SLOW[name] ?: 60L)
        return when (name) {
            "status" -> statusJson()
            "live" -> liveJson()
            "codes" -> codesJson()
            "clear" -> if (a != "yes") err("clear", "Send 'clear yes' to confirm") else {
                stored.clear(); pending.clear()
                JSONObject().put("t", "clear").put("ok", true).put("msg", "Codes cleared. Cycle the key and check the FI light.")
            }
            "freeze" -> if (stored.isEmpty())
                JSONObject().put("t", "freeze").put("ok", true).put("code", JSONObject.NULL).put("msg", "No freeze frame stored")
            else JSONObject().put("t", "freeze").put("ok", true).put("code", stored[0]).put("desc", desc(stored[0]))
                .put("vals", JSONObject().put("ect", 88.0).put("iat", 33.0).put("tps", 18.4).put("map", 41.0)
                    .put("spd", 62.0).put("adv", 27.5).put("stft", -9.4).put("ltft", 2.3).put("o2", 0.06))
            "pids" -> JSONObject().put("t", "pids").put("ok", true).put("ktm_rpm", true)
                .put("list", JSONArray(listOf("01", "03", "04", "05", "06", "07", "0B", "0C", "0D", "0E", "0F", "11", "13", "14", "1C", "21", "30", "31")))
            "vin" -> JSONObject().put("t", "vin").put("ok", true).put("vin", "VBKJYJ40XDC000000")
            "ident" -> items("ident", listOf(
                "ECU name (09 0A)" to "KTM 390 DUKE ECU", "Calibration ID (09 04)" to "KM390DK1-DEMO",
                "Software version (1A 94)" to "SW 0.0.DEMO", "Hardware (1A 92)" to null))
            "ktm" -> items("ktm", listOf("21 01" to "DEMO 01", "21 22" to "DEMO 22", "21 25" to null)).put("rpm", sample(now())["rpm"]!!.toInt())
            "scan" -> items("scan", listOf("21 01" to "DEMO 01", "21 21" to "1A 2C", "21 22" to "DEMO 22"))
            "discover" -> items("discover", listOf("10 81 (default session)" to "50 81", "10 85 (programming)" to null, "27 01 (security)" to null))
            "monitor" -> {
                val ids = JSONArray()
                for ((id, d) in busFrames(now())) ids.put(JSONObject().put("id", id).put("n", 300).put("data", d.joinToString(" ") { hex(it) }).put("chg", id != "540"))
                JSONObject().put("t", "monitor").put("ok", true).put("ids", ids)
            }
            "raw" -> if (settings["allow_raw"] != "1") err("raw", "Raw commands are off. Turn on Allow raw ECU commands in Settings.")
            else if (a.isEmpty()) err("raw", "Give hex bytes, e.g. raw 22 F1 90")
            else {
                val req = a.uppercase().split(" ").filter { it.isNotEmpty() }
                val resp = listOf(hex(((req[0].toIntOrNull(16) ?: 0) + 0x40) and 0xFF)) + req.drop(1) + listOf("00", "2A")
                JSONObject().put("t", "raw").put("ok", true).put("req", req.joinToString(" ")).put("hex", resp.joinToString(" ")).put("ascii", "...*")
            }
            "peek" -> peek(a)
            "sniff" -> sniff(a)
            "modules" -> JSONObject().put("t", "modules").put("ok", true).put("skipped", 0).put("list", JSONArray()
                .put(JSONObject().put("tx", "7E0").put("rx", "7E8").put("name", "Engine ECU").put("info", "KTM 390 DUKE ECU (demo)"))
                .put(JSONObject().put("tx", "7E1").put("rx", "7E9").put("name", "Module").put("info", "ABS (demo)")))
            "mcodes" -> {
                val tx = a.uppercase().removePrefix("@")
                when (tx) {
                    "7E0" -> mcodes(tx, "7E8", "KWP2000", stored)
                    "7E1" -> mcodes(tx, "7E9", "UDS", absCodes)
                    else -> err("mcodes", "No reply from $tx")
                }
            }
            "mclear" -> {
                val tx = a.substringBefore(' ').uppercase()
                if (!a.endsWith("yes")) err("mclear", "Send 'mclear $tx yes' to confirm")
                else {
                    if (tx == "7E0") { stored.clear(); pending.clear() } else absCodes.clear()
                    JSONObject().put("t", "mclear").put("ok", true).put("tx", tx).put("msg", "Codes cleared on $tx")
                }
            }
            "gear" -> gear(a)
            "peak" -> { pkRpm = 0.0; pkSpd = 0.0; pkEct = -1000.0; JSONObject().put("t", "peak").put("ok", true).put("msg", "Peaks reset") }
            "settings" -> settingsJson()
            "set" -> set(a)
            "log" -> log(a)
            "logrows" -> logRows(a)
            "reboot" -> JSONObject().put("t", "reboot").put("ok", true).put("msg", "Rebooting (demo: leaving demo mode)")
            "factory" -> if (a != "yes") err("factory", "Send 'factory yes' to confirm") else {
                factoryReset(); JSONObject().put("t", "factory").put("ok", true).put("msg", "Settings reset (demo: leaving demo mode)")
            }
            else -> err(name, "Unknown command $name")
        }.toString()
    }

    override suspend fun logCsv(): String {
        check()
        delay(400)
        val n = logCount()
        val ms = logMs()
        val sb = StringBuilder(logHeader())
        for (i in 0 until n) sb.append(logRow((logStart - t0) / 1000.0 + i * ms / 1000.0, i * ms / 1000.0))
        return sb.toString()
    }

    private fun check() { if (!connected) throw WifiLink.LinkException("Demo mode is off") }

    // ---------------- replies ----------------
    private fun err(t: String, msg: String) = JSONObject().put("t", t).put("ok", false).put("msg", msg)

    private fun statusJson(): JSONObject {
        val sigs = JSONArray()
        for (i in 1..4) if (sigOn(i)) {
            val f = settings["sig$i"]!!.split(":")
            sigs.put(JSONObject().put("k", "c$i").put("n", f.getOrElse(6) { "Signal $i" }).put("u", f.getOrElse(7) { "" }))
        }
        return JSONObject().put("t", "status").put("ok", true).put("fw", "1.2.0 (demo)")
            .put("ecu", true).put("ecu_id", "7E8").put("can", "500k").put("ktm_rpm", true)
            .put("fi", if (stored.isEmpty()) 0 else 1).put("dtc", stored.size)
            .put("uptime", (now() + 600).toLong()).put("heap", 143360).put("tracker", false)
            .put("bt_mac", "DE:30:00:00:00:01").put("wifi_clients", 0).put("ap_ip", settings["ap_ip"]).put("sta_ip", "")
            .put("log", logOn).put("log_n", logCount()).put("log_max", LOG_MAX)
            .put("units", settings["units"]!!.toInt()).put("live_ms", settings["live_ms"]!!.toInt())
            .put("temp_warn", settings["temp_warn"]!!.toInt()).put("rpm_warn", settings["rpm_warn"]!!.toInt())
            .put("vbat_low", settings["vbat_low"]!!.toDouble()).put("raw", settings["allow_raw"] == "1")
            .put("bt_client", false).put("bt_mode", "none")
            .put("gears", JSONArray(gears().map { round(it, 1) })).put("sigs", sigs)
    }

    private fun codesJson(): JSONObject {
        fun list(c: List<String>) = JSONArray().apply { c.forEach { put(JSONObject().put("code", it).put("desc", desc(it))) } }
        return JSONObject().put("t", "codes").put("ok", true).put("fi", stored.isNotEmpty()).put("count", stored.size)
            .put("km_fi", if (stored.isEmpty()) 0 else 37).put("km_clr", if (stored.isEmpty()) 0 else 412).put("warmups", 18)
            .put("stored", list(stored)).put("pending", list(pending))
    }

    private fun mcodes(tx: String, rx: String, proto: String, codes: List<String>): JSONObject {
        val a = JSONArray()
        codes.forEach { a.put(JSONObject().put("code", if (proto == "UDS") "$it-11" else it).put("st", "08").put("desc", desc(it))) }
        return JSONObject().put("t", "mcodes").put("ok", true).put("tx", tx).put("rx", rx).put("proto", proto).put("list", a)
    }

    private fun items(t: String, list: List<Pair<String, String?>>): JSONObject {
        val a = JSONArray()
        for ((k, v) in list) {
            if (v == null) a.put(JSONObject().put("k", k).put("msg", "no reply"))
            else a.put(JSONObject().put("k", k).put("hex", v.toByteArray().joinToString(" ") { hex(it.toInt() and 0xFF) }).put("ascii", v))
        }
        return JSONObject().put("t", t).put("ok", true).put("items", a)
    }

    private fun peek(a: String): JSONObject {
        val req = a.uppercase().split(" ").filter { it.isNotEmpty() }
        if (req.isEmpty()) return err("peek", "Give a request in hex, e.g. 21 22")
        val svc = req[0].removePrefix("@")
        if (svc !in listOf("01", "02", "09", "1A", "21", "22")) return err("peek", "Watch only allows read requests (01 02 09 1A 21 22)")
        val s = sample(now())
        val rpm = s["rpm"]!!.toInt()
        val body = listOf(rpm shr 8 and 0xFF, rpm and 0xFF, s["tps"]!!.toInt() * 255 / 100, s["ect"]!!.toInt() + 40, 0x12, s["spd"]!!.toInt(), 0x00, 0x5A)
        val resp = listOf(hex((svc.toInt(16) + 0x40) and 0xFF)) + req.drop(1) + body.map { hex(it and 0xFF) }
        return JSONObject().put("t", "peek").put("ok", true).put("req", req.joinToString(" ")).put("hex", resp.joinToString(" "))
    }

    /** Pretend broadcast traffic on the bike's CAN bus. */
    private fun busFrames(t: Double): List<Pair<String, List<Int>>> {
        val s = sample(t)
        val rpm = s["rpm"]!!.toInt()
        val f = (s["spd"]!! * 100).toInt()
        val r = (s["spd"]!! * 102).toInt()
        val tick = (t * 10).toInt() and 0xFF
        return listOf(
            "120" to listOf(tick, 0x00, f shr 8 and 0xFF, f and 0xFF, r shr 8 and 0xFF, r and 0xFF, 0x00, 0x00),
            "12B" to listOf(rpm shr 8 and 0xFF, rpm and 0xFF, s["tps"]!!.toInt() * 2, s["gear"]!!.toInt(), 0x00, 0x00, 0x00, tick),
            "540" to listOf(0x01, 0x40, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00),
            "550" to listOf(s["ect"]!!.toInt() + 40, s["iat"]!!.toInt() + 40, (s["vbat"]!! * 10).toInt(), 0x00, 0x00, 0x00, 0x00, 0x00),
            "7E8" to listOf(0x04, 0x41, 0x0C, rpm * 4 shr 8 and 0xFF, rpm * 4 and 0xFF, 0x00, 0x00, 0x00)
        )
    }

    private fun sniff(a: String): JSONObject {
        val t = now()
        when (a) {
            "start" -> { sniffOn = true; sniffStart = System.currentTimeMillis(); sniffMark.clear(); sniffPrev.clear() }
            "stop" -> sniffOn = false
            "clear" -> { sniffStart = System.currentTimeMillis(); sniffMark.clear(); sniffPrev.clear() }
            "mark" -> sniffMark.clear()
        }
        val ids = JSONArray()
        if (sniffOn || a == "stop") for ((id, d) in busFrames(t)) {
            val m = sniffMark.getOrPut(id) { (d + d).toIntArray() }
            var mk = 0
            var chg = 0
            val prev = sniffPrev[id]
            d.forEachIndexed { i, b ->
                if (b < m[i]) m[i] = b
                if (b > m[i + d.size]) m[i + d.size] = b
                if (m[i] != m[i + d.size]) mk = mk or (1 shl i)
                if (prev != null && prev[i] != b) chg = chg or (1 shl i)
            }
            sniffPrev[id] = d
            val hz = when (id) { "120", "12B" -> 100.0; "7E8" -> 6.5; else -> 10.0 }
            val secs = (System.currentTimeMillis() - sniffStart) / 1000.0
            ids.put(JSONObject().put("id", id).put("n", (hz * secs).toInt() + 1).put("hz", hz).put("age", 10)
                .put("d", d.joinToString(" ") { hex(it) }).put("c", chg).put("m", mk)
                .put("lo", (0 until d.size).joinToString(" ") { hex(m[it]) })
                .put("hi", (0 until d.size).joinToString(" ") { hex(m[it + d.size]) }))
        }
        return JSONObject().put("t", "sniff").put("ok", true).put("on", sniffOn)
            .put("ms", if (sniffOn) System.currentTimeMillis() - sniffStart else 0).put("full", false).put("ids", ids)
    }

    private fun gear(a: String): JSONObject {
        if (a == "reset") settings["gears"] = DEFS.first { it.key == "gears" }.def
        else if (a.startsWith("learn")) {
            val g = a.substringAfter("learn").trim().toIntOrNull() ?: return err("gear", "Which gear? e.g. gear learn 2")
            if (g !in 1..6) return err("gear", "Gear must be 1-6")
            val s = sample(now())
            if (s["spd"]!! < 15) return err("gear", "Ride at a steady speed above 15 km/h in that gear first")
            val list = gears().toMutableList()
            list[g - 1] = round(s["rpm"]!! / s["spd"]!!, 1)
            settings["gears"] = list.joinToString(",")
        }
        return JSONObject().put("t", "gear").put("ok", true).put("ratios", JSONArray(gears().map { round(it, 1) }))
    }

    private fun settingsJson(): JSONObject {
        val a = JSONArray()
        for (d in DEFS) {
            val o = JSONObject().put("k", d.key).put("l", d.label).put("g", d.group).put("t", d.type)
                .put("r", d.reboot).put("min", d.min).put("max", d.max)
            if (d.type == "pass") o.put("v", "").put("set", settings[d.key]!!.isNotEmpty()) else o.put("v", settings[d.key])
            if (d.type == "sel") o.put("o", d.opts)
            a.put(o)
        }
        return JSONObject().put("t", "settings").put("ok", true).put("fields", a)
    }

    private fun set(args: String): JSONObject {
        val key = args.substringBefore(' ')
        var v = if (args.contains(' ')) args.substringAfter(' ').trim() else ""
        val d = DEFS.firstOrNull { it.key == key } ?: return err("set", "Unknown setting $key")
        when (d.type) {
            "int", "sel" -> {
                val n = v.toLongOrNull() ?: return err("set", "${d.label}: whole number needed")
                if (n < d.min || n > d.max) return err("set", "${d.label}: must be ${d.min.toInt()}-${d.max.toInt()}")
                v = n.toString()
            }
            "float" -> {
                val f = v.toDoubleOrNull()
                if (f == null || f < d.min || f > d.max) return err("set", "${d.label}: must be ${d.min}-${d.max}")
                v = String.format(java.util.Locale.US, "%.2f", f)
            }
            "bool" -> v = if (v.lowercase() in listOf("1", "true", "on", "yes")) "1" else "0"
            else -> {
                if (v.length > d.max) return err("set", "${d.label}: max ${d.max.toInt()} characters")
                if (key.startsWith("sig") && v.isNotEmpty() && v.split(":").size < 6)
                    return err("set", "Signal ${key.last()}: Format: SOURCE:byte:length:b/l:scale:offset:name:unit")
                if (key == "gears" && v.split(",").size != 6) return err("set", "Gear ratios: 6 numbers separated by commas")
            }
        }
        settings[key] = v
        return JSONObject().put("t", "set").put("ok", true).put("k", key).put("reboot", d.reboot)
    }

    // ---------------- logging ----------------
    private fun logMs() = (settings["log_ms"]?.toIntOrNull() ?: 1000).coerceAtLeast(200)

    private fun logCount(): Int =
        if (!logOn) logFrozenN else min(LOG_MAX, ((System.currentTimeMillis() - logStart) / logMs()).toInt() + 1)

    private fun log(a: String): JSONObject {
        when (a) {
            "start" -> if (!logOn) { logOn = true; logStart = System.currentTimeMillis() }
            "stop" -> { logFrozenN = logCount(); logOn = false }
            "clear" -> { logFrozenN = 0; if (logOn) logStart = System.currentTimeMillis() }
        }
        return JSONObject().put("t", "log").put("ok", true).put("on", logOn).put("n", logCount()).put("max", LOG_MAX).put("ms", logMs())
    }

    private fun logHeader(): String {
        val sb = StringBuilder("seconds,rpm,tps_pct,map_kpa,speed_kmh,coolant_c,intake_c,advance_deg,stft_pct,ltft_pct,o2_v,battery_v,gear")
        for (i in 1..4) {
            if (sigOn(i)) {
                val f = settings["sig$i"]!!.split(":")
                sb.append(",").append(f.getOrElse(6) { "signal$i" }.replace(",", " "))
                f.getOrNull(7)?.takeIf { it.isNotEmpty() }?.let { sb.append("_").append(it) }
            } else sb.append(",signal$i")
        }
        return sb.append("\n").toString()
    }

    private fun logRow(t: Double, secs: Double): String {
        val s = sample(t)
        val sb = StringBuilder(String.format(java.util.Locale.US, "%.1f", secs))
        for ((k, dp) in LIVE_DP) {
            sb.append(',')
            val v = s[k] ?: Double.NaN
            if (!v.isNaN() && (!k.startsWith("c") || sigOn(k.substring(1).toInt())))
                sb.append(String.format(java.util.Locale.US, "%.${min(dp, 3)}f", v))
        }
        return sb.append('\n').toString()
    }

    private fun logRows(a: String): JSONObject {
        val parts = a.split(" ")
        val start = parts.getOrNull(0)?.toIntOrNull()?.coerceAtLeast(0) ?: 0
        val cnt = (parts.getOrNull(1)?.toIntOrNull() ?: 40).coerceIn(1, 80)
        val total = logCount()
        val ms = logMs()
        val rows = StringBuilder()
        var n = 0
        var i = start
        while (i < total && n < cnt) { rows.append(logRow((logStart - t0) / 1000.0 + i * ms / 1000.0, i * ms / 1000.0)); i++; n++ }
        val o = JSONObject().put("t", "logrows").put("ok", true).put("total", total).put("start", start).put("n", n)
        if (start == 0) o.put("header", logHeader())
        return o.put("rows", rows.toString())
    }

    private class Def(
        val key: String, val label: String, val group: String, val type: String, val def: String,
        val reboot: Boolean, val opts: String, val min: Double, val max: Double
    )

    companion object {
        private const val LOG_MAX = 600

        private fun hex(b: Int) = "%02X".format(b and 0xFF)

        /** Live keys and decimal places, in the firmware's order. */
        private val LIVE_DP = listOf(
            "rpm" to 0, "tps" to 1, "map" to 0, "spd" to 0, "ect" to 0, "iat" to 0, "adv" to 1, "stft" to 1,
            "ltft" to 1, "o2" to 3, "vbat" to 2, "gear" to 0, "c1" to 2, "c2" to 2, "c3" to 2, "c4" to 2
        )

        /** Slower replies for the commands that take a while on the real unit. */
        private val SLOW = mapOf("codes" to 700L, "freeze" to 600L, "modules" to 2500L, "mcodes" to 900L, "ident" to 1500L,
            "ktm" to 800L, "scan" to 3000L, "discover" to 900L, "monitor" to 3000L, "clear" to 800L, "mclear" to 800L)

        private val DESCS = mapOf(
            "P0130" to "O2 sensor circuit", "P0505" to "Idle control system",
            "C0035" to "Front wheel speed sensor circuit"
        )

        private fun desc(code: String) = DESCS[code.substringBefore('-')] ?: ""

        /** Same settings as the firmware (keys, labels, groups, defaults, limits). */
        private val DEFS = listOf(
            Def("wifi_mode", "Wi-Fi mode", "Wi-Fi", "sel", "0", true, "0:Hotspot only|1:Hotspot + home Wi-Fi|2:Wi-Fi off", 0.0, 2.0),
            Def("ap_ssid", "Hotspot name", "Wi-Fi", "str", "OBDuke", true, "", 1.0, 32.0),
            Def("ap_pass", "Hotspot password (8+ chars, blank = open)", "Wi-Fi", "pass", "obduke1234", true, "", 0.0, 63.0),
            Def("ap_ip", "Hotspot address (web page IP)", "Wi-Fi", "str", "1.1.1.1", true, "", 7.0, 15.0),
            Def("ap_ch", "Hotspot channel", "Wi-Fi", "int", "6", true, "", 1.0, 13.0),
            Def("ap_hidden", "Hide hotspot name", "Wi-Fi", "bool", "0", true, "", 0.0, 1.0),
            Def("sta_ssid", "Home Wi-Fi name", "Wi-Fi", "str", "", true, "", 0.0, 32.0),
            Def("sta_pass", "Home Wi-Fi password", "Wi-Fi", "pass", "", true, "", 0.0, 63.0),
            Def("hostname", "Network name (.local)", "Wi-Fi", "str", "obduke", true, "", 1.0, 24.0),
            Def("web_user", "Web login user", "Web page", "str", "admin", false, "", 1.0, 24.0),
            Def("web_pass", "Web login password (blank = no login)", "Web page", "pass", "", false, "", 0.0, 32.0),
            Def("captive", "Open page automatically on join", "Web page", "bool", "1", true, "", 0.0, 1.0),
            Def("bt_name", "Bluetooth name (Torque / tracker see this)", "Bluetooth ELM327", "str", "OBDuke", true, "", 1.0, 24.0),
            Def("trk_on", "Bluetooth ELM327 link (Torque Pro / tracker)", "Bluetooth ELM327", "bool", "1", true, "", 0.0, 1.0),
            Def("trk_pin", "Bluetooth pairing PIN", "Bluetooth ELM327", "str", "1234", true, "", 4.0, 16.0),
            Def("trk_virt", "Virtual PIDs (RPM, battery, gear, custom signals)", "Bluetooth ELM327", "bool", "1", false, "", 0.0, 1.0),
            Def("can_rate", "CAN speed", "CAN / ECU", "sel", "0", true, "0:Auto|1:500 kbps|2:250 kbps", 0.0, 2.0),
            Def("can_tx", "CAN TX pin (to CTX)", "CAN / ECU", "int", "21", true, "", 0.0, 33.0),
            Def("can_rx", "CAN RX pin (to CRX)", "CAN / ECU", "int", "22", true, "", 0.0, 39.0),
            Def("rpm_div", "KTM RPM divisor", "CAN / ECU", "float", "4", false, "", 1.0, 16.0),
            Def("allow_raw", "Allow raw ECU commands (advanced)", "CAN / ECU", "bool", "0", false, "", 0.0, 1.0),
            Def("units", "Units", "Display", "sel", "0", false, "0:Metric|1:Imperial", 0.0, 1.0),
            Def("live_ms", "Live data refresh (ms)", "Display", "int", "150", false, "", 100.0, 5000.0),
            Def("temp_warn", "Coolant warning (C)", "Display", "int", "105", false, "", 60.0, 130.0),
            Def("rpm_warn", "Shift / RPM warning", "Display", "int", "9500", false, "", 1000.0, 12000.0),
            Def("gears", "Gear ratios, rpm per km/h (1st..6th)", "Gear indicator", "str", "175,131,99,82,73,67", false, "", 0.0, 63.0),
            Def("sig1", "Signal 1 (SOURCE:byte:length:b/l:scale:offset:name:unit)", "Custom signals", "str", "", false, "", 0.0, 63.0),
            Def("sig2", "Signal 2", "Custom signals", "str", "", false, "", 0.0, 63.0),
            Def("sig3", "Signal 3", "Custom signals", "str", "", false, "", 0.0, 63.0),
            Def("sig4", "Signal 4", "Custom signals", "str", "", false, "", 0.0, 63.0),
            Def("vbat_pin", "Battery sense pin (0 = off, use 32-39)", "Battery", "int", "0", true, "", 0.0, 39.0),
            Def("vbat_mult", "Battery divider ratio", "Battery", "float", "5.7", false, "", 1.0, 20.0),
            Def("vbat_low", "Low battery warning (V)", "Battery", "float", "11.8", false, "", 9.0, 14.0),
            Def("log_ms", "Log interval (ms)", "Logging", "int", "1000", false, "", 200.0, 60000.0),
            Def("log_auto", "Start logging on power-up", "Logging", "bool", "0", false, "", 0.0, 1.0),
            Def("led_pin", "Status LED pin (0 = off)", "System", "int", "2", true, "", 0.0, 33.0),
        )
    }
}
