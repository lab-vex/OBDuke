package au.obduke.app

import android.app.Application
import android.content.Context
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import org.json.JSONArray
import org.json.JSONObject

data class Dtc(val code: String, val desc: String, val pending: Boolean)

data class Module(val tx: String, val rx: String, val name: String, val info: String)

data class SigDef(val key: String, val name: String, val unit: String)

/** One row of the bus sniffer or watch view. chg = changed just now, mk = changed since Mark. */
data class ByteRow(
    val id: String, val hz: String, val bytes: List<String>, val chg: Int, val mk: Int,
    val ranges: String, val stale: Boolean
)

data class Field(
    val key: String, val label: String, val group: String, val type: String,
    val value: String, val reboot: Boolean, val options: List<Pair<String, String>>,
    val isSet: Boolean
)

enum class LinkState { IDLE, CONNECTING, CONNECTED }

class ObdukeViewModel(app: Application) : AndroidViewModel(app) {

    private val prefs = app.getSharedPreferences("obduke", Context.MODE_PRIVATE)
    val wifi = WifiLink(app)
    val bt = BtLink(app)

    /** "bt" = Bluetooth (default), "wifi" = the OBDuke hotspot. */
    val transport = mutableStateOf(prefs.getString("transport", "bt")!!)
    val btAddr = mutableStateOf(prefs.getString("btaddr", "")!!)
    val btDevices = mutableStateListOf<Pair<String, String>>()
    /** Set by MainActivity once the Bluetooth permission is granted (always true below Android 12). */
    val btAllowed = mutableStateOf(false)

    val link: ObdLink get() = if (transport.value == "bt") bt else wifi
    val linkName get() = if (transport.value == "bt") "Bluetooth" else "Wi-Fi"

    // connection
    val linkState = mutableStateOf(LinkState.IDLE)
    val connectMsg = mutableStateOf("")
    val needLogin = mutableStateOf(false)
    val toast = mutableStateOf<String?>(null)
    val busy = mutableStateOf<String?>(null)

    // app-side settings (how to reach the unit)
    val ssid = mutableStateOf(prefs.getString("ssid", "OBDuke")!!)
    val wifiPass = mutableStateOf(prefs.getString("wpass", "obduke1234")!!)
    // Firmware 1.1 moved the hotspot from 192.168.4.1 to 1.1.1.1. The app also falls back to
    // the hotspot's gateway address, so either firmware works.
    val host = mutableStateOf(
        prefs.getString("host", DEFAULT_HOST)!!.let { if (it == "192.168.4.1") DEFAULT_HOST else it }
    )
    val webUser = mutableStateOf(prefs.getString("wuser", "admin")!!)
    val webPass = mutableStateOf(prefs.getString("wpw", "")!!)
    val autoConnect = mutableStateOf(prefs.getBoolean("auto", true))
    val keepScreenOn = mutableStateOf(prefs.getBoolean("screen", true))

    // device status
    val status = mutableStateOf(JSONObject())
    val live = mutableStateMapOf<String, Double?>()
    val ecu = mutableStateOf(false)
    val fi = mutableStateOf(-1)
    val dtcCount = mutableStateOf(-1)
    val units = mutableStateOf(0)
    val tempWarn = mutableStateOf(105.0)
    val rpmWarn = mutableStateOf(9500.0)
    val vbatLow = mutableStateOf(11.8)
    val peaks = mutableStateMapOf<String, Double>()
    val sigDefs = mutableStateListOf<SigDef>()
    val gears = mutableStateOf<List<Double>>(emptyList())
    private var liveMs = 150L

    // codes
    val codesRead = mutableStateOf(false)
    val dtcs = mutableStateListOf<Dtc>()
    val codesInfo = mutableStateOf("")
    val freeze = mutableStateOf<JSONObject?>(null)

    // other modules (ABS etc)
    val modules = mutableStateListOf<Module>()
    val modulesScanned = mutableStateOf(false)
    val modCodesTitle = mutableStateOf("")
    val modCodes = mutableStateListOf<Dtc>()
    val modCodesMsg = mutableStateOf("")

    // sniffer + watch
    val sniffOn = mutableStateOf(false)
    val sniffRows = mutableStateListOf<ByteRow>()
    val sniffHideDiag = mutableStateOf(true)
    val sniffOnlyMarked = mutableStateOf(false)
    val sniffMsg = mutableStateOf("Not running.")
    val watchOn = mutableStateOf(false)
    val watchReq = mutableStateOf("21 22")
    val watchRow = mutableStateOf<ByteRow?>(null)
    val watchMsg = mutableStateOf("Not running.")
    private var watchPrev: IntArray? = null
    private var watchLo: IntArray? = null
    private var watchHi: IntArray? = null
    private var sniffJob: Job? = null
    private var watchJob: Job? = null

    // tools / log / settings
    val toolOut = mutableStateOf("Pick a tool.")
    val logInfo = mutableStateOf(JSONObject())
    val fields = mutableStateListOf<Field>()
    var onCsvReady: (String) -> Unit = {}

    private var liveJob: Job? = null
    private var failStreak = 0

    init { applyLinkSettings() }

    private fun applyLinkSettings() {
        wifi.host = host.value.trim()
        wifi.user = webUser.value.trim()
        wifi.pass = webPass.value
        wifi.onHostFound = { h ->
            host.value = h
            prefs.edit().putString("host", h).apply()
        }
    }

    fun saveLinkSettings() {
        prefs.edit()
            .putString("ssid", ssid.value).putString("wpass", wifiPass.value)
            .putString("host", host.value).putString("wuser", webUser.value).putString("wpw", webPass.value)
            .putString("transport", transport.value).putString("btaddr", btAddr.value)
            .apply()
        applyLinkSettings()
    }

    fun setPref(key: String, v: Boolean) { prefs.edit().putBoolean(key, v).apply() }

    fun setTransport(t: String) {
        transport.value = t
        prefs.edit().putString("transport", t).apply()
        if (t == "bt") refreshBtDevices()
    }

    fun refreshBtDevices() {
        btDevices.clear()
        if (!btAllowed.value) return
        btDevices.addAll(bt.pairedDevices())
        // first time: pick the paired OBDuke unit automatically
        if (btDevices.none { it.second == btAddr.value }) {
            btDevices.firstOrNull { it.first.contains("obduke", true) }?.let { btAddr.value = it.second }
        }
    }

    fun pickBt(address: String) {
        btAddr.value = address
        prefs.edit().putString("btaddr", address).apply()
    }

    fun btPermissionResult(granted: Boolean) {
        btAllowed.value = granted
        if (granted) refreshBtDevices()
    }

    // ---------------- connection ----------------
    fun connect() {
        if (linkState.value == LinkState.CONNECTING) return
        saveLinkSettings()
        linkState.value = LinkState.CONNECTING
        val useBt = transport.value == "bt"
        connectMsg.value = if (useBt) "Connecting over Bluetooth…" else "Looking for ${ssid.value}…"
        viewModelScope.launch {
            try {
                if (useBt && !btAllowed.value) throw WifiLink.LinkException("Allow the Bluetooth permission first")
                val s = if (useBt) bt.connect(btAddr.value) else wifi.connect(ssid.value, wifiPass.value)
                handle(s)
                failStreak = 0
                linkState.value = LinkState.CONNECTED
                connectMsg.value = ""
            } catch (e: WifiLink.AuthException) {
                linkState.value = LinkState.IDLE
                needLogin.value = true
                connectMsg.value = "OBDuke wants the web login."
            } catch (e: Exception) {
                link.disconnect()
                linkState.value = LinkState.IDLE
                connectMsg.value = e.message ?: "Couldn't connect"
            }
        }
    }

    fun tryAutoConnect() {
        if (!autoConnect.value || linkState.value != LinkState.IDLE) return
        if (transport.value == "bt" && (!btAllowed.value || btAddr.value.isBlank() || !bt.isOn)) return
        connect()
    }

    fun disconnect() {
        stopLive()
        sniffJob?.cancel(); sniffOn.value = false
        watchJob?.cancel(); watchOn.value = false
        wifi.disconnect()
        bt.disconnect()
        linkState.value = LinkState.IDLE
        ecu.value = false
        busy.value = null
    }

    private fun lost(msg: String) {
        disconnect()
        connectMsg.value = msg
    }

    fun login(user: String, pass: String) {
        webUser.value = user
        webPass.value = pass
        needLogin.value = false
        saveLinkSettings()
        connect()
    }

    // ---------------- commands ----------------
    fun send(cmd: String, busyLabel: String? = null, timeoutMs: Int = 15000) {
        if (busyLabel != null) busy.value = busyLabel
        viewModelScope.launch {
            try {
                handle(link.cmd(cmd, timeoutMs))
                failStreak = 0
            } catch (e: WifiLink.AuthException) {
                needLogin.value = true
            } catch (e: Exception) {
                toast.value = e.message ?: "Request failed"
                if (++failStreak >= 3) lost("Lost connection to OBDuke")
            } finally {
                if (busyLabel != null) busy.value = null
            }
        }
    }

    fun startLive() {
        if (liveJob?.isActive == true) return
        liveJob = viewModelScope.launch {
            while (isActive && linkState.value == LinkState.CONNECTED) {
                val t0 = System.currentTimeMillis()
                try {
                    handle(link.live())
                    failStreak = 0
                } catch (e: WifiLink.AuthException) {
                    needLogin.value = true
                    break
                } catch (e: Exception) {
                    if (++failStreak >= 4) { lost("Lost connection to OBDuke"); break }
                }
                delay((liveMs.coerceAtLeast(100) - (System.currentTimeMillis() - t0)).coerceAtLeast(10))
            }
        }
    }

    fun stopLive() { liveJob?.cancel(); liveJob = null }

    fun downloadCsv() {
        busy.value = "Downloading log"
        viewModelScope.launch {
            try { onCsvReady(link.logCsv()) }
            catch (e: Exception) { toast.value = e.message ?: "Download failed" }
            finally { busy.value = null }
        }
    }

    // ---------------- incoming JSON ----------------
    private fun handle(text: String) {
        val j = try { JSONObject(text) } catch (e: Exception) { return }
        val t = j.optString("t")

        if (!j.optBoolean("ok", true)) {
            val msg = j.optString("msg")
            if (t in TOOL_TYPES) toolOut.value = "Error: $msg"
            if (t == "mcodes") { modCodes.clear(); modCodesTitle.value = "MODULE CODES"; modCodesMsg.value = msg }
            toast.value = msg
            return
        }

        when (t) {
            "status" -> {
                status.value = j
                ecu.value = j.optBoolean("ecu")
                fi.value = j.optInt("fi", -1)
                dtcCount.value = j.optInt("dtc", -1)
                units.value = j.optInt("units", 0)
                tempWarn.value = j.optDouble("temp_warn", 105.0)
                rpmWarn.value = j.optDouble("rpm_warn", 9500.0)
                vbatLow.value = j.optDouble("vbat_low", 11.8)
                liveMs = j.optLong("live_ms", 500)
                val sg = j.optJSONArray("sigs")
                sigDefs.clear()
                if (sg != null) for (i in 0 until sg.length()) {
                    val o = sg.getJSONObject(i)
                    sigDefs.add(SigDef(o.optString("k"), o.optString("n"), o.optString("u")))
                }
                j.optJSONArray("gears")?.let { a -> gears.value = (0 until a.length()).map { a.optDouble(it) } }
            }
            "live" -> {
                ecu.value = j.optBoolean("ecu")
                fi.value = j.optInt("fi", -1)
                dtcCount.value = j.optInt("dtc", -1)
                for (k in LIVE_KEYS) live[k] = if (!j.has(k) || j.isNull(k)) null else j.optDouble(k)
                j.optJSONObject("pk")?.let { p ->
                    for (k in listOf("rpm", "spd", "ect")) if (p.has(k) && !p.isNull(k)) peaks[k] = p.optDouble(k) else peaks.remove(k)
                }
            }
            "codes" -> {
                codesRead.value = true
                dtcs.clear()
                parseDtcs(j.optJSONArray("stored"), false)
                parseDtcs(j.optJSONArray("pending"), true)
                if (j.has("fi")) fi.value = if (j.optBoolean("fi")) 1 else 0
                val bits = mutableListOf<String>()
                if (j.has("count")) bits += "${j.optInt("count")} code(s) reported"
                if (j.has("km_fi")) bits += "${j.optInt("km_fi")} km with light on"
                if (j.has("km_clr")) bits += "${j.optInt("km_clr")} km since clear"
                codesInfo.value = bits.joinToString(" · ")
            }
            "clear" -> { toast.value = "Codes nuked. Hail to the king, baby."; send("codes", "Reading codes") }
            "freeze" -> freeze.value = j
            in TOOL_TYPES -> toolOut.value = formatTool(j)
            "settings" -> parseSettings(j.optJSONArray("fields"))
            "set" -> {}
            "log" -> logInfo.value = j
            "modules" -> {
                modulesScanned.value = true
                modules.clear()
                val a = j.optJSONArray("list") ?: JSONArray()
                for (i in 0 until a.length()) {
                    val o = a.getJSONObject(i)
                    modules.add(Module(o.optString("tx"), o.optString("rx"), o.optString("name"), o.optString("info")))
                }
            }
            "mcodes" -> {
                modCodes.clear()
                modCodesTitle.value = "MODULE ${j.optString("tx")} CODES (${j.optString("proto")})"
                val a = j.optJSONArray("list") ?: JSONArray()
                for (i in 0 until a.length()) {
                    val o = a.getJSONObject(i)
                    val st = o.optString("st")
                    modCodes.add(Dtc(o.optString("code"), o.optString("desc").ifEmpty { if (st.isNotEmpty()) "status $st" else "" }, false))
                }
                modCodesMsg.value = if (modCodes.isEmpty()) "No codes stored." else ""
            }
            "mclear" -> { toast.value = j.optString("msg"); send("mcodes ${j.optString("tx")}", "Reading module codes") }
            "gear" -> j.optJSONArray("ratios")?.let { a -> gears.value = (0 until a.length()).map { a.optDouble(it) } }
            "peak" -> { peaks.clear(); toast.value = j.optString("msg") }
            "reboot", "factory" -> { toast.value = j.optString("msg"); lost("OBDuke is rebooting. Tap Connect in a few seconds.") }
        }
    }

    private fun parseDtcs(a: JSONArray?, pending: Boolean) {
        if (a == null) return
        for (i in 0 until a.length()) {
            val o = a.getJSONObject(i)
            dtcs.add(Dtc(o.optString("code"), o.optString("desc"), pending))
        }
    }

    private fun parseSettings(a: JSONArray?) {
        if (a == null) return
        fields.clear()
        for (i in 0 until a.length()) {
            val o = a.getJSONObject(i)
            val opts = o.optString("o").split("|").filter { it.contains(":") }
                .map { val p = it.split(":", limit = 2); p[0] to p[1] }
            fields.add(
                Field(
                    o.optString("k"), o.optString("l"), o.optString("g"), o.optString("t"),
                    o.optString("v"), o.optBoolean("r"), opts, o.optBoolean("set")
                )
            )
        }
    }

    private fun formatTool(j: JSONObject): String {
        return when (j.optString("t")) {
            "pids" -> {
                val l = j.optJSONArray("list")
                val s = (0 until (l?.length() ?: 0)).joinToString(" ") { l!!.getString(it) }
                "Supported mode 01 PIDs:\n$s\n\nKTM extended RPM (2121): ${if (j.optBoolean("ktm_rpm")) "yes" else "no"}"
            }
            "vin" -> "VIN: ${j.optString("vin")}"
            "raw" -> "Sent: ${j.optString("req")}\nGot:  ${j.optString("hex")}\n      |${j.optString("ascii")}|"
            "monitor" -> {
                val a = j.optJSONArray("ids") ?: JSONArray()
                if (a.length() == 0) "(no broadcast traffic)" else
                    "ID   count  data\n" + (0 until a.length()).joinToString("\n") {
                        val o = a.getJSONObject(it)
                        o.optString("id").padEnd(5) + o.optInt("n").toString().padEnd(7) +
                            o.optString("data") + if (o.optBoolean("chg")) "  *" else ""
                    }
            }
            else -> {
                val a = j.optJSONArray("items") ?: JSONArray()
                val body = if (a.length() == 0) "Nothing answered." else (0 until a.length()).joinToString("\n\n") {
                    val o = a.getJSONObject(it)
                    if (o.has("msg")) "${o.optString("k")}\n  ${o.optString("msg")}"
                    else "${o.optString("k")}\n  ${o.optString("hex")}\n  |${o.optString("ascii")}|"
                }
                body + if (j.has("rpm")) "\n\nRPM: ${j.optInt("rpm")}" else ""
            }
        }
    }

    // ---------------- bus sniffer ----------------
    fun sniffToggle() {
        if (sniffOn.value) {
            sniffOn.value = false
            sniffJob?.cancel()
            send("sniff stop")
            return
        }
        sniffOn.value = true
        sniffMsg.value = "Listening…"
        sniffRows.clear()
        sniffJob?.cancel()
        sniffJob = viewModelScope.launch {
            var first = true
            while (isActive && sniffOn.value && linkState.value == LinkState.CONNECTED) {
                try {
                    parseSniff(JSONObject(link.cmd(if (first) "sniff start" else "sniff", 5000)))
                    first = false
                } catch (e: WifiLink.AuthException) { needLogin.value = true; break }
                catch (_: Exception) {}
                delay(400)
            }
        }
    }

    fun sniffPause() { sniffJob?.cancel(); if (sniffOn.value) { sniffOn.value = false; send("sniff stop") } }

    private var lastSniff: JSONObject? = null

    fun sniffRedraw() { lastSniff?.let { parseSniff(it) } }

    private fun parseSniff(j: JSONObject) {
        lastSniff = j
        val a = j.optJSONArray("ids") ?: JSONArray()
        val rows = mutableListOf<ByteRow>()
        for (i in 0 until a.length()) {
            val o = a.getJSONObject(i)
            val id = o.optString("id")
            val mk = o.optInt("m")
            if (sniffHideDiag.value && id.length == 3 && id.startsWith("7")) continue
            if (sniffOnlyMarked.value && mk == 0) continue
            val lo = o.optString("lo").split(" ")
            val hi = o.optString("hi").split(" ")
            val rg = lo.indices.filter { ((mk shr it) and 1) == 1 }.joinToString("  ") { "b$it ${lo[it]}-${hi.getOrElse(it) { "" }}" }
            rows.add(
                ByteRow(
                    id, "${o.optString("hz")} Hz", o.optString("d").split(" ").filter { it.isNotEmpty() },
                    o.optInt("c"), mk, rg, o.optLong("age") > 2000
                )
            )
        }
        rows.sortBy { it.id.toLongOrNull(16) ?: 0L }
        sniffRows.clear()
        sniffRows.addAll(rows)
        sniffMsg.value = when {
            rows.isNotEmpty() -> if (j.optBoolean("full")) "Table full (64 IDs). Use the filters or Clear." else ""
            j.optBoolean("on") -> "Listening… no frames yet."
            else -> "Not running."
        }
    }

    // ---------------- watch a request ----------------
    fun watchToggle() {
        if (watchOn.value) { watchOn.value = false; watchJob?.cancel(); return }
        watchOn.value = true
        watchPrev = null
        watchMark()
        watchJob?.cancel()
        watchJob = viewModelScope.launch {
            while (isActive && watchOn.value && linkState.value == LinkState.CONNECTED) {
                try {
                    val j = JSONObject(link.cmd("peek ${watchReq.value}", 5000))
                    if (!j.optBoolean("ok")) watchMsg.value = j.optString("msg")
                    else parseWatch(j)
                } catch (e: WifiLink.AuthException) { needLogin.value = true; break }
                catch (_: Exception) {}
                delay(300)
            }
        }
    }

    fun watchPause() { watchJob?.cancel(); watchOn.value = false }

    fun watchMark() { watchLo = null; watchHi = null }

    private fun parseWatch(j: JSONObject) {
        val hex = j.optString("hex").split(" ").filter { it.isNotEmpty() }
        val b = hex.map { it.toInt(16) }.toIntArray()
        if (watchLo == null || watchLo!!.size != b.size) { watchLo = b.copyOf(); watchHi = b.copyOf() }
        val lo = watchLo!!
        val hi = watchHi!!
        var chg = 0
        var mk = 0
        val prev = watchPrev
        for (i in b.indices) {
            if (prev != null && i < prev.size && prev[i] != b[i]) chg = chg or (1 shl minOf(i, 30))
            if (b[i] < lo[i]) lo[i] = b[i]
            if (b[i] > hi[i]) hi[i] = b[i]
            if (lo[i] != hi[i]) mk = mk or (1 shl minOf(i, 30))
        }
        watchPrev = b
        val rg = b.indices.filter { lo[it] != hi[it] }
            .joinToString("\n") { "byte $it: %02X-%02X (%d-%d)".format(lo[it], hi[it], lo[it], hi[it]) }
        watchRow.value = ByteRow(j.optString("req"), "", hex, chg, mk, rg + (if (j.has("nrc")) "\nECU says: ${j.optString("nrc")}" else ""), false)
        watchMsg.value = ""
    }

    override fun onCleared() { wifi.disconnect(); bt.disconnect() }

    companion object {
        const val DEFAULT_HOST = "1.1.1.1"
        val LIVE_KEYS = listOf("rpm", "tps", "map", "spd", "ect", "iat", "adv", "stft", "ltft", "o2", "vbat",
            "gear", "c1", "c2", "c3", "c4")
        val TOOL_TYPES = setOf("pids", "vin", "ident", "ktm", "scan", "discover", "monitor", "raw")
    }
}
