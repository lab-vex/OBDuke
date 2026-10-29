package au.obduke.app

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.abs

// =====================================================================
//  Connect screen
// =====================================================================
@Composable
fun ConnectScreen(vm: ObdukeViewModel, onAskBt: () -> Unit, onBtSettings: () -> Unit) {
    val st = vm.linkState.value
    val useBt = vm.transport.value == "bt"
    var showAdv by remember { mutableStateOf(false) }
    LaunchedEffect(vm.transport.value, vm.btAllowed.value) { if (useBt) vm.refreshBtDevices() }

    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(16.dp)) {
        CardBox {
            Text(
                if (st == LinkState.CONNECTING) "Connecting…" else "Not connected",
                fontWeight = FontWeight.ExtraBold, fontSize = 18.sp
            )
            Text(
                vm.connectMsg.value.ifEmpty {
                    if (useBt) "Power the OBDuke unit, pick it below and tap Connect."
                    else "Power the OBDuke unit, then tap Connect. Your phone may ask to join the OBDuke Wi-Fi — allow it."
                },
                color = C.Muted, fontSize = 13.sp, modifier = Modifier.padding(top = 4.dp)
            )
            if (st == LinkState.CONNECTING)
                LinearProgressIndicator(Modifier.fillMaxWidth().padding(top = 12.dp), color = C.Orange, trackColor = C.Card2)
            Spacer(Modifier.height(12.dp))
            KButton(
                "Connect", { vm.connect() }, primary = true,
                enabled = st == LinkState.IDLE && (!useBt || (vm.btAllowed.value && vm.btAddr.value.isNotBlank())),
                modifier = Modifier.fillMaxWidth()
            )
        }

        Heading("Connect over")
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            KButton("Bluetooth", { vm.setTransport("bt") }, primary = useBt, modifier = Modifier.weight(1f), enabled = st == LinkState.IDLE)
            KButton("Wi-Fi", { vm.setTransport("wifi") }, primary = !useBt, modifier = Modifier.weight(1f), enabled = st == LinkState.IDLE)
        }

        if (useBt) {
            Heading("OBDuke unit")
            CardBox {
                when {
                    !vm.bt.hasBluetooth -> Text("This phone has no Bluetooth. Use Wi-Fi instead.", color = C.Muted)
                    !vm.btAllowed.value -> {
                        Text("OBDuke needs permission to use Bluetooth.", color = C.Muted)
                        Spacer(Modifier.height(8.dp))
                        KButton("Allow Bluetooth", onAskBt, primary = true)
                    }
                    else -> {
                        if (!vm.bt.isOn) {
                            Text("Bluetooth is off.", color = C.Warn, fontWeight = FontWeight.Bold)
                            Spacer(Modifier.height(8.dp))
                        }
                        if (vm.btDevices.isEmpty())
                            Text("No paired devices yet. Pair OBDuke first (below).", color = C.Muted)
                        vm.btDevices.forEach { (name, addr) ->
                            val sel = addr == vm.btAddr.value
                            Row(
                                Modifier.fillMaxWidth().clip(RoundedCornerShape(8.dp)).clickable { vm.pickBt(addr) }.padding(vertical = 6.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                RadioButton(sel, { vm.pickBt(addr) }, colors = RadioButtonDefaults.colors(selectedColor = C.Orange))
                                Column(Modifier.weight(1f)) {
                                    Text(name, fontWeight = if (sel) FontWeight.ExtraBold else FontWeight.Normal)
                                    Text(addr, color = C.Muted, fontSize = 11.sp, fontFamily = FontFamily.Monospace)
                                }
                            }
                        }
                        Spacer(Modifier.height(8.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            KButton("Pair new", onBtSettings, modifier = Modifier.weight(1f))
                            KButton("Refresh", { vm.refreshBtDevices() }, modifier = Modifier.weight(1f))
                        }
                    }
                }
            }
            Text(
                "First time: tap Pair new, pick OBDuke in your phone's Bluetooth list and enter PIN 1234, then come back. " +
                    "The unit takes one Bluetooth connection at a time, so disconnect Torque (and the tracker, if it's in range) first. Needs unit firmware 1.2.",
                color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 8.dp)
            )
        } else {
            Heading("Hotspot")
            CardBox {
                OutlinedTextField(vm.ssid.value, { vm.ssid.value = it }, label = { Text("Wi-Fi name") }, singleLine = true, modifier = Modifier.fillMaxWidth())
                Spacer(Modifier.height(8.dp))
                OutlinedTextField(
                    vm.wifiPass.value, { vm.wifiPass.value = it }, label = { Text("Wi-Fi password") }, singleLine = true,
                    visualTransformation = PasswordVisualTransformation(), modifier = Modifier.fillMaxWidth()
                )
                TextButton({ showAdv = !showAdv }) { Text(if (showAdv) "Hide advanced" else "Advanced", color = C.Orange) }
                if (showAdv) {
                    OutlinedTextField(vm.host.value, { vm.host.value = it }, label = { Text("Address") }, singleLine = true, modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(8.dp))
                    OutlinedTextField(vm.webUser.value, { vm.webUser.value = it }, label = { Text("Web login user") }, singleLine = true, modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(8.dp))
                    OutlinedTextField(
                        vm.webPass.value, { vm.webPass.value = it }, label = { Text("Web login password (if set)") }, singleLine = true,
                        visualTransformation = PasswordVisualTransformation(), modifier = Modifier.fillMaxWidth()
                    )
                    Text("Address is 1.1.1.1 on the hotspot (192.168.4.1 on firmware 1.0 — the app finds either). On home Wi-Fi use the IP shown in the unit's Settings > About.",
                        color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
                }
            }
        }
    }
}

// =====================================================================
//  Dashboard
// =====================================================================
private data class Gauge(
    val key: String, val label: String, val unit: String, val max: Double, val dp: Int,
    val big: Boolean = false, val absBar: Boolean = false,
    val imperial: ((Double) -> Double)? = null, val impUnit: String = ""
)

private val GAUGES = listOf(
    Gauge("rpm", "RPM", "", 15000.0, 0, big = true),
    Gauge("tps", "Throttle", "%", 100.0, 1),
    Gauge("spd", "Speed", "km/h", 180.0, 0, imperial = { it * 0.6214 }, impUnit = "mph"),
    Gauge("ect", "Coolant", "°C", 120.0, 0, imperial = { it * 1.8 + 32 }, impUnit = "°F"),
    Gauge("iat", "Intake air", "°C", 80.0, 0, imperial = { it * 1.8 + 32 }, impUnit = "°F"),
    Gauge("map", "MAP", "kPa", 110.0, 0),
    Gauge("adv", "Ign advance", "°", 50.0, 1),
    Gauge("stft", "Short trim", "%", 25.0, 1, absBar = true),
    Gauge("ltft", "Long trim", "%", 25.0, 1, absBar = true),
    Gauge("o2", "O2 sensor", "V", 1.0, 2),
    Gauge("vbat", "Battery", "V", 15.0, 2),
    Gauge("gear", "Gear", "", 6.0, 0),
)

@Composable
fun DashScreen(vm: ObdukeViewModel) {
    DisposableEffect(Unit) {
        vm.startLive()
        onDispose { vm.stopLive() }
    }
    val imp = vm.units.value == 1
    Column(Modifier.fillMaxSize().padding(horizontal = 16.dp)) {
        if (vm.fi.value == 1) {
            Spacer(Modifier.height(12.dp))
            CardBox(borderColor = C.Warn) {
                Text("FI LIGHT IS ON — ${vm.dtcCount.value} code(s). See Codes.", color = C.Warn, fontWeight = FontWeight.ExtraBold)
            }
        } else if (!vm.ecu.value) {
            Text("Waiting for ECU — turn the key on.", color = C.Muted, modifier = Modifier.padding(top = 12.dp))
        }
        LazyVerticalGrid(
            columns = GridCells.Fixed(2),
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
            contentPadding = PaddingValues(vertical = 12.dp)
        ) {
            val all = GAUGES + vm.sigDefs.map { Gauge(it.key, it.name, it.unit, 0.0, 2) }
            items(all, span = { GridItemSpan(if (it.big) 2 else 1) }) { g ->
                val raw = vm.live[g.key]
                val warn = raw != null && when (g.key) {
                    "ect" -> raw >= vm.tempWarn.value
                    "rpm" -> raw >= vm.rpmWarn.value
                    "vbat" -> raw < vm.vbatLow.value
                    else -> false
                }
                val shown = raw?.let { if (imp && g.imperial != null) g.imperial.invoke(it) else it }
                val unit = if (imp && g.imperial != null) g.impUnit else g.unit
                val pct = if (g.max <= 0) 0.0 else raw?.let { ((if (g.absBar) abs(it) else it) / g.max).coerceIn(0.0, 1.0) } ?: 0.0
                val pk = vm.peaks[g.key]?.let { p -> "max %.0f".format(if (imp && g.imperial != null) g.imperial.invoke(p) else p) }
                val text = when {
                    shown == null -> "--"
                    g.key == "gear" -> if (shown < 0.5) "–" else "%.0f".format(shown)
                    else -> "%.${g.dp}f".format(shown)
                }
                if (g.key == "rpm") RpmTile(shown, pct.toFloat(), warn, pk)
                else Tile(g.label, text, unit, pct.toFloat(), warn, g.big, raw == null, pk)
            }
            item(span = { GridItemSpan(2) }) {
                Row { KButton("Reset peaks", { vm.send("peak reset") }) }
            }
        }
    }
}

/** RPM tile with a 0-15 (x1000) scale under a horizontal bar. Red from 10k. */
@Composable
private fun RpmTile(rpm: Double?, pct: Float, warn: Boolean, peak: String? = null) {
    Column(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(C.Card)
            .border(1.dp, if (warn) C.Bad else C.Line, RoundedCornerShape(12.dp))
            .padding(start = 14.dp, end = 14.dp, top = 12.dp, bottom = 12.dp)
    ) {
        Text("RPM", color = C.Muted, fontSize = 11.sp, fontWeight = FontWeight.Bold)
        Row(verticalAlignment = Alignment.Bottom) {
            Text(
                rpm?.let { "%.0f".format(it) } ?: "--", fontSize = 44.sp, fontWeight = FontWeight.ExtraBold,
                color = when { rpm == null -> Color(0xFF555555); warn -> C.Bad; else -> C.Text }
            )
            Spacer(Modifier.weight(1f))
            Column(horizontalAlignment = Alignment.End) {
                if (peak != null) Text(peak, color = C.Muted, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                Text("×1000 rpm", color = C.Muted, fontSize = 10.sp, modifier = Modifier.padding(bottom = 6.dp))
            }
        }
        // scale labels + ticks
        BoxWithConstraints(Modifier.fillMaxWidth().height(26.dp).padding(horizontal = 4.dp)) {
            val w = maxWidth
            for (i in 0..15) {
                val x = w * (i / 15f)
                val red = i >= 10
                Text(
                    "$i", fontSize = 11.sp, fontWeight = FontWeight.Bold,
                    color = if (red) C.Bad else C.Muted,
                    modifier = Modifier.offset(x = x - 6.dp, y = 0.dp).width(12.dp),
                    textAlign = androidx.compose.ui.text.style.TextAlign.Center
                )
                Box(
                    Modifier.offset(x = x, y = 17.dp).width(1.dp).height(8.dp)
                        .background(if (red) C.Bad else Color(0xFF555555))
                )
            }
        }
        Box(Modifier.fillMaxWidth().height(12.dp).clip(RoundedCornerShape(3.dp)).background(Color(0xFF262626))) {
            Box(Modifier.fillMaxWidth(pct).fillMaxHeight().background(if (warn) C.Bad else C.Orange))
        }
    }
}

@Composable
private fun Tile(label: String, value: String, unit: String, pct: Float, warn: Boolean, big: Boolean, na: Boolean, peak: String? = null) {
    val accent = if (warn) C.Bad else C.Orange
    Box(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(C.Card)
            .border(1.dp, if (warn) C.Bad else C.Line, RoundedCornerShape(12.dp))
    ) {
        Column(Modifier.padding(start = 14.dp, end = 14.dp, top = 12.dp, bottom = 16.dp)) {
            Text(label.uppercase(), color = C.Muted, fontSize = 11.sp, fontWeight = FontWeight.Bold)
            Row(verticalAlignment = Alignment.Bottom) {
                Text(
                    value, fontSize = if (big) 44.sp else 30.sp, fontWeight = FontWeight.ExtraBold,
                    color = when { na -> Color(0xFF555555); warn -> C.Bad; else -> C.Text }
                )
                Text(" $unit", color = C.Muted, fontSize = 13.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.padding(bottom = 6.dp))
            }
            if (peak != null) Text(peak, color = C.Muted, fontSize = 11.sp, fontWeight = FontWeight.Bold)
        }
        Box(
            Modifier.align(Alignment.BottomStart).fillMaxWidth(pct).height(4.dp).background(accent)
        )
    }
}

// =====================================================================
//  Codes
// =====================================================================
@Composable
fun CodesScreen(vm: ObdukeViewModel) {
    var confirm by remember { mutableStateOf(false) }
    var clearMod by remember { mutableStateOf<String?>(null) }
    LaunchedEffect(Unit) { if (!vm.codesRead.value) vm.send("codes", "Reading codes") }

    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(16.dp)) {
        CardBox {
            Row(verticalAlignment = Alignment.CenterVertically) {
                val lampColor = when (vm.fi.value) { 1 -> C.Warn; 0 -> Color(0xFF1A3D22); else -> Color(0xFF222222) }
                val ring = when (vm.fi.value) { 1 -> Color(0xFFFFE066); 0 -> C.Ok; else -> Color(0xFF333333) }
                Box(Modifier.size(46.dp).clip(CircleShape).background(lampColor).border(3.dp, ring, CircleShape))
                Spacer(Modifier.width(14.dp))
                Column {
                    Text(
                        "FI light: " + when (vm.fi.value) { 1 -> "ON"; 0 -> "off"; else -> "unknown" },
                        fontWeight = FontWeight.ExtraBold, fontSize = 18.sp
                    )
                    Text(vm.codesInfo.value.ifEmpty { "Tap Read codes" }, color = C.Muted, fontSize = 13.sp)
                }
            }
        }
        Spacer(Modifier.height(12.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            KButton("Read", { vm.send("codes", "Reading codes") }, primary = true, modifier = Modifier.weight(1f))
            KButton("Freeze", { vm.send("freeze", "Reading freeze frame") }, modifier = Modifier.weight(1f))
            KButton("Clear", { confirm = true }, danger = true, modifier = Modifier.weight(1f))
        }
        Spacer(Modifier.height(12.dp))
        CardBox {
            if (!vm.codesRead.value) Text("No codes read yet.", color = C.Muted)
            else if (vm.dtcs.isEmpty()) Text("No stored or pending codes.", color = C.Ok, fontWeight = FontWeight.ExtraBold)
            vm.dtcs.forEachIndexed { i, d ->
                if (i > 0) HorizontalDivider(color = C.Line, modifier = Modifier.padding(vertical = 8.dp))
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(d.code, color = C.Orange, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.ExtraBold, fontSize = 18.sp, modifier = Modifier.width(80.dp))
                    Text(d.desc.ifEmpty { "Unknown code" }, modifier = Modifier.weight(1f))
                    Text(
                        if (d.pending) "PENDING" else "STORED", fontSize = 11.sp, fontWeight = FontWeight.Bold, color = C.Muted,
                        modifier = Modifier.clip(RoundedCornerShape(6.dp)).background(C.Card2).padding(horizontal = 8.dp, vertical = 3.dp)
                    )
                }
            }
        }
        vm.freeze.value?.let { f ->
            Spacer(Modifier.height(12.dp))
            CardBox {
                if (f.isNull("code")) Text(f.optString("msg", "No freeze frame stored"), color = C.Muted)
                else {
                    Text("FREEZE FRAME — ${f.optString("code")}", color = C.Orange, fontWeight = FontWeight.ExtraBold)
                    Text(f.optString("desc"), color = C.Muted, fontSize = 13.sp, modifier = Modifier.padding(bottom = 8.dp))
                    val names = mapOf(
                        "ect" to "Coolant °C", "iat" to "Intake °C", "tps" to "Throttle %", "map" to "MAP kPa",
                        "spd" to "Speed km/h", "adv" to "Advance °", "stft" to "Short trim %", "ltft" to "Long trim %", "o2" to "O2 V"
                    )
                    val v = f.optJSONObject("vals")
                    v?.keys()?.forEach { k -> KV(names[k] ?: k, v.optString(k)) }
                }
            }
        }

        Heading("ABS & other modules")
        KButton("Find modules", { vm.send("modules", "Scanning modules (~10 s)", 60000) })
        Text("Key on, engine off. Pings every diagnostic address, then you can read or clear each module's codes.",
            color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
        if (vm.modulesScanned.value) {
            Spacer(Modifier.height(10.dp))
            CardBox {
                if (vm.modules.isEmpty()) Text("Nothing answered. Is the key on?", color = C.Muted)
                vm.modules.forEachIndexed { i, m ->
                    if (i > 0) HorizontalDivider(color = C.Line, modifier = Modifier.padding(vertical = 8.dp))
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text(m.tx, color = C.Orange, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.ExtraBold, modifier = Modifier.width(48.dp))
                        Column(Modifier.weight(1f)) {
                            Text(m.name, fontWeight = FontWeight.ExtraBold)
                            Text(m.info.ifEmpty { "replies on ${m.rx}" }, color = C.Muted, fontSize = 12.sp)
                        }
                    }
                    Spacer(Modifier.height(6.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        KButton("Read codes", { vm.send("mcodes ${m.tx}", "Reading module codes") }, modifier = Modifier.weight(1f))
                        KButton("Clear", { clearMod = m.tx }, danger = true, modifier = Modifier.weight(1f))
                    }
                }
            }
        }
        if (vm.modCodesTitle.value.isNotEmpty()) {
            Spacer(Modifier.height(10.dp))
            CardBox {
                Text(vm.modCodesTitle.value, color = C.Orange, fontWeight = FontWeight.ExtraBold)
                if (vm.modCodesMsg.value.isNotEmpty())
                    Text(vm.modCodesMsg.value, color = if (vm.modCodes.isEmpty() && vm.modCodesMsg.value == "No codes stored.") C.Ok else C.Muted, modifier = Modifier.padding(top = 6.dp))
                vm.modCodes.forEach { d ->
                    Row(Modifier.padding(top = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                        Text(d.code, color = C.Orange, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.ExtraBold, fontSize = 18.sp, modifier = Modifier.width(110.dp))
                        Text(d.desc.ifEmpty { "See the KTM manual for this module" }, modifier = Modifier.weight(1f))
                    }
                }
            }
        }
        Spacer(Modifier.height(24.dp))
    }

    clearMod?.let { tx ->
        AlertDialog(
            onDismissRequest = { clearMod = null },
            containerColor = C.Card,
            title = { Text("Clear module $tx?", fontWeight = FontWeight.ExtraBold) },
            text = { Text("Clears all stored codes in this module. Key on, engine off.") },
            confirmButton = { TextButton({ clearMod = null; vm.send("mclear $tx yes", "Clearing module codes") }) { Text("CLEAR", color = C.Bad, fontWeight = FontWeight.ExtraBold) } },
            dismissButton = { TextButton({ clearMod = null }) { Text("CANCEL", color = C.Muted) } }
        )
    }

    if (confirm) AlertDialog(
        onDismissRequest = { confirm = false },
        containerColor = C.Card,
        title = { Text("Clear all codes?", fontWeight = FontWeight.ExtraBold) },
        text = { Text("This clears stored and pending codes and turns the FI light off. Key on, engine off works best.") },
        confirmButton = { TextButton({ confirm = false; vm.send("clear yes", "Clearing codes") }) { Text("CLEAR", color = C.Bad, fontWeight = FontWeight.ExtraBold) } },
        dismissButton = { TextButton({ confirm = false }) { Text("CANCEL", color = C.Muted) } }
    )
}

@Composable
fun KV(k: String, v: String) {
    Row(Modifier.fillMaxWidth().padding(vertical = 3.dp)) {
        Text(k, color = C.Muted, modifier = Modifier.weight(1f))
        Text(v, fontWeight = FontWeight.SemiBold)
    }
}

// =====================================================================
//  Tools
// =====================================================================
@Composable
fun ToolsScreen(vm: ObdukeViewModel) {
    var raw by remember { mutableStateOf("") }
    DisposableEffect(Unit) {
        vm.send("status")
        onDispose { vm.sniffPause(); vm.watchPause() }
    }
    val rawAllowed = vm.status.value.optBoolean("raw")
    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(16.dp)) {
        Heading("Info", Modifier.padding(top = 0.dp))
        FlowButtons(listOf(
            "Supported PIDs" to { vm.send("pids", "Reading PIDs") },
            "VIN" to { vm.send("vin", "Reading VIN") },
            "ECU identity" to { vm.send("ident", "Reading ECU identity", 40000) },
            "Service check" to { vm.send("discover", "Checking services") },
        ))
        Heading("KTM / reverse engineering")
        FlowButtons(listOf(
            "KTM extended" to { vm.send("ktm", "Reading KTM IDs") },
            "Scan local IDs" to { vm.send("scan", "Scanning (about 1 min)", 120000) },
            "Bus monitor 3s" to { vm.send("monitor 3000", "Listening for 3 s", 20000) },
        ))

        Heading("Bus sniffer")
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            KButton(if (vm.sniffOn.value) "Stop" else "Start", { vm.sniffToggle() }, primary = true, modifier = Modifier.weight(1f))
            KButton("Mark", { vm.send("sniff mark"); vm.toast.value = "Marked. Now spin a wheel / move a lever." }, modifier = Modifier.weight(1f))
            KButton("Clear", { vm.send("sniff clear") }, modifier = Modifier.weight(1f))
        }
        CheckRow("Hide diagnostic replies (7xx)", vm.sniffHideDiag.value) { vm.sniffHideDiag.value = it; vm.sniffRedraw() }
        CheckRow("Only IDs changed since Mark", vm.sniffOnlyMarked.value) { vm.sniffOnlyMarked.value = it; vm.sniffRedraw() }
        Text("Orange box = changed just now, orange text = changed since Mark. Tap Mark, spin the front wheel by hand and watch which bytes light up. Found one? Add it in Settings > Custom signals.",
            color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(vertical = 6.dp))
        MonoBox {
            if (vm.sniffMsg.value.isNotEmpty()) Text(vm.sniffMsg.value, color = C.Muted, fontSize = 12.sp)
            vm.sniffRows.forEach { ByteRowView(it) }
        }

        Heading("Watch a request")
        Row(verticalAlignment = Alignment.CenterVertically) {
            OutlinedTextField(vm.watchReq.value, { vm.watchReq.value = it }, Modifier.weight(1f), placeholder = { Text("21 22  or  @7B0 21 05") }, singleLine = true)
            Spacer(Modifier.width(8.dp))
            KButton(if (vm.watchOn.value) "Stop" else "Watch", { vm.watchToggle() }, primary = true)
        }
        Row(Modifier.padding(top = 6.dp)) { KButton("Mark", { vm.watchMark() }) }
        Text("Reads the same ECU value over and over and highlights bytes that move. Byte 0 is the reply code; for Custom signals / Torque subtract 2 from the byte number. Read-only requests only.",
            color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(vertical = 6.dp))
        MonoBox {
            if (vm.watchMsg.value.isNotEmpty()) Text(vm.watchMsg.value, color = C.Muted, fontSize = 12.sp)
            vm.watchRow.value?.let { ByteRowView(it) }
        }

        Heading("Gear calibration")
        Text("Ride steady above 15 km/h, clutch out, and tap the gear you're in. Only needed if the Dash shows the wrong gear.",
            color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(bottom = 6.dp))
        FlowButtons((1..6).map { g -> "$g" to { vm.send("gear learn $g", "Saving gear $g") } } + listOf("Defaults" to { vm.send("gear reset") }))
        if (vm.gears.value.isNotEmpty())
            Text("rpm per km/h: " + vm.gears.value.mapIndexed { i, r -> "${i + 1}=%.0f".format(r) }.joinToString("  "),
                fontFamily = FontFamily.Monospace, color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
        Heading("Raw command")
        Row(verticalAlignment = Alignment.CenterVertically) {
            OutlinedTextField(raw, { raw = it }, Modifier.weight(1f), placeholder = { Text("e.g. 22 F1 90") }, singleLine = true)
            Spacer(Modifier.width(8.dp))
            KButton("Send", { vm.send("raw $raw", "Sending") }, enabled = rawAllowed && raw.isNotBlank())
        }
        if (!rawAllowed) Text("Turn on \"Allow raw ECU commands\" in Settings.", color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
        Heading("Output")
        Text(
            vm.toolOut.value, fontFamily = FontFamily.Monospace, fontSize = 12.sp, color = Color(0xFFDDDDDD),
            modifier = Modifier.fillMaxWidth().clip(RoundedCornerShape(10.dp)).background(Color.Black)
                .border(1.dp, C.Line, RoundedCornerShape(10.dp)).padding(12.dp)
        )
    }
}

@Composable
private fun CheckRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(Modifier.fillMaxWidth().clickable { onChange(!checked) }.padding(vertical = 2.dp), verticalAlignment = Alignment.CenterVertically) {
        Checkbox(checked, onChange, colors = CheckboxDefaults.colors(checkedColor = C.Orange, checkmarkColor = Color.Black))
        Text(label, color = C.Muted, fontSize = 13.sp)
    }
}

@Composable
private fun MonoBox(content: @Composable ColumnScope.() -> Unit) {
    Column(
        Modifier.fillMaxWidth().clip(RoundedCornerShape(10.dp)).background(Color.Black)
            .border(1.dp, C.Line, RoundedCornerShape(10.dp)).padding(10.dp),
        content = content
    )
}

/** ID, rate, then each byte: orange box = changed just now, orange text = changed since Mark. */
@Composable
private fun ByteRowView(r: ByteRow) {
    Column(Modifier.fillMaxWidth().padding(vertical = 4.dp).alpha(if (r.stale) 0.45f else 1f)) {
        Text(
            buildAnnotatedString {
                if (r.hz.isNotEmpty()) {
                    withStyle(SpanStyle(color = C.Orange, fontWeight = FontWeight.ExtraBold)) { append(r.id.padEnd(4)) }
                    withStyle(SpanStyle(color = C.Muted, fontSize = 10.sp)) { append(" " + r.hz.padEnd(8)) }
                }
                r.bytes.forEachIndexed { i, b ->
                    val now = i < 31 && ((r.chg shr i) and 1) == 1
                    val mk = i < 31 && ((r.mk shr i) and 1) == 1
                    when {
                        now -> withStyle(SpanStyle(background = C.Orange, color = Color.Black, fontWeight = FontWeight.Bold)) { append(b) }
                        mk -> withStyle(SpanStyle(color = C.Orange, fontWeight = FontWeight.Bold)) { append(b) }
                        else -> withStyle(SpanStyle(color = Color(0xFFDDDDDD))) { append(b) }
                    }
                    append(" ")
                }
            },
            fontFamily = FontFamily.Monospace, fontSize = 13.sp
        )
        if (r.ranges.isNotEmpty()) Text(r.ranges, fontFamily = FontFamily.Monospace, fontSize = 11.sp, color = C.Muted)
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun FlowButtons(items: List<Pair<String, () -> Unit>>) {
    FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
        items.forEach { (t, f) -> KButton(t, f) }
    }
}

// =====================================================================
//  Log
// =====================================================================
@Composable
fun LogScreen(vm: ObdukeViewModel, onExport: () -> Unit) {
    LaunchedEffect(Unit) { vm.send("log status") }
    val l = vm.logInfo.value
    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(16.dp)) {
        CardBox {
            val on = l.optBoolean("on")
            Row { Text("Status", color = C.Muted, modifier = Modifier.weight(1f)); Text(if (on) "LOGGING" else "Stopped", color = if (on) C.Ok else C.Muted, fontWeight = FontWeight.ExtraBold) }
            KV("Samples", "${l.optInt("n")} / ${l.optInt("max")}")
            KV("Interval", "${l.optInt("ms")} ms")
            KV("Capacity", "${l.optInt("max") * l.optInt("ms") / 60000} min")
        }
        Spacer(Modifier.height(12.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            KButton("Start", { vm.send("log start") }, primary = true, modifier = Modifier.weight(1f))
            KButton("Stop", { vm.send("log stop") }, modifier = Modifier.weight(1f))
            KButton("Clear", { vm.send("log clear") }, modifier = Modifier.weight(1f))
        }
        Spacer(Modifier.height(8.dp))
        KButton("Export CSV", onExport, modifier = Modifier.fillMaxWidth())
        Text(
            "Logs RPM, throttle, MAP, speed, temps, advance, fuel trims, O2 and battery into the unit's memory. Lost on power off — export before you unplug.",
            color = C.Muted, fontSize = 13.sp, modifier = Modifier.padding(top = 12.dp)
        )
    }
}

// =====================================================================
//  Settings
// =====================================================================
@Composable
fun SettingsScreen(vm: ObdukeViewModel) {
    LaunchedEffect(Unit) { vm.send("settings"); vm.send("status") }
    val edits = remember { mutableStateMapOf<String, String>() }
    var askReboot by remember { mutableStateOf(false) }
    var askFactory by remember { mutableStateOf(false) }

    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(16.dp)) {
        Heading("App", Modifier.padding(top = 0.dp))
        CardBox {
            SwitchRow("Auto-connect to last unit", vm.autoConnect.value) { vm.autoConnect.value = it; vm.setPref("auto", it) }
            SwitchRow("Keep screen on", vm.keepScreenOn.value) { vm.keepScreenOn.value = it; vm.setPref("screen", it) }
            Spacer(Modifier.height(8.dp))
            KV("Connected over", vm.linkName)
            Spacer(Modifier.height(8.dp))
            KButton("Disconnect", { vm.disconnect() })
        }

        var group = ""
        vm.fields.forEach { f ->
            if (f.group != group) { group = f.group; Heading(group) }
            SettingField(f, edits[f.key]) { edits[f.key] = it }
        }
        if (vm.fields.isEmpty()) Text("Loading settings…", color = C.Muted, modifier = Modifier.padding(top = 12.dp))
        Text("↻ = takes effect after reboot", color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 10.dp))

        Spacer(Modifier.height(14.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            KButton("Save", {
                var reboot = false
                edits.forEach { (k, v) ->
                    val f = vm.fields.firstOrNull { it.key == k } ?: return@forEach
                    if (f.type != "pass" && v == f.value) return@forEach
                    if (f.type == "pass" && v.isEmpty()) return@forEach
                    vm.send("set $k $v")
                    if (f.reboot) reboot = true
                }
                edits.clear()
                vm.send("settings"); vm.send("status")
                vm.toast.value = "Saved"
                if (reboot) askReboot = true
            }, primary = true, modifier = Modifier.weight(1f))
            KButton("Reboot", { vm.send("reboot") }, modifier = Modifier.weight(1f))
            KButton("Reset", { askFactory = true }, danger = true, modifier = Modifier.weight(1f))
        }

        Heading("About")
        CardBox {
            val s = vm.status.value
            KV("Firmware", s.optString("fw", "—"))
            KV("ECU", if (s.optBoolean("ecu")) "Connected (${s.optString("ecu_id")}, ${s.optString("can")})" else "Not connected")
            KV("KTM RPM", if (s.optBoolean("ktm_rpm")) "Yes" else "No")
            KV("Bluetooth (Torque / tracker)", if (s.optBoolean("tracker")) "Connected" else "Not connected")
            KV("Hotspot address", s.optString("ap_ip", "—"))
            KV("Phones on hotspot", s.optInt("wifi_clients").toString())
            KV("Bluetooth MAC", s.optString("bt_mac", "—"))
            KV("Home Wi-Fi IP", s.optString("sta_ip").ifEmpty { "—" })
            KV("Uptime", "${s.optLong("uptime") / 60} min")
            KV("Free memory", "${s.optLong("heap") / 1024} KB")
            KV("App", "1.2.0")
        }
        Spacer(Modifier.height(24.dp))
    }

    if (askReboot) AlertDialog(
        onDismissRequest = { askReboot = false }, containerColor = C.Card,
        title = { Text("Reboot now?", fontWeight = FontWeight.ExtraBold) },
        text = { Text("Some changes only take effect after a reboot. If you changed the hotspot name or password, update them on the Connect screen too.") },
        confirmButton = { TextButton({ askReboot = false; vm.send("reboot") }) { Text("REBOOT", color = C.Orange, fontWeight = FontWeight.ExtraBold) } },
        dismissButton = { TextButton({ askReboot = false }) { Text("LATER", color = C.Muted) } }
    )
    if (askFactory) AlertDialog(
        onDismissRequest = { askFactory = false }, containerColor = C.Card,
        title = { Text("Factory reset?", fontWeight = FontWeight.ExtraBold) },
        text = { Text("All settings go back to defaults and the unit reboots.") },
        confirmButton = { TextButton({ askFactory = false; vm.send("factory yes") }) { Text("RESET", color = C.Bad, fontWeight = FontWeight.ExtraBold) } },
        dismissButton = { TextButton({ askFactory = false }) { Text("CANCEL", color = C.Muted) } }
    )
}

@Composable
private fun SwitchRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(Modifier.fillMaxWidth().padding(vertical = 4.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(label, modifier = Modifier.weight(1f))
        Switch(
            checked, onChange,
            colors = SwitchDefaults.colors(checkedThumbColor = Color.Black, checkedTrackColor = C.Orange)
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun SettingField(f: Field, edit: String?, onEdit: (String) -> Unit) {
    val label = f.label + if (f.reboot) " ↻" else ""
    val value = edit ?: if (f.type == "pass") "" else f.value
    Box(Modifier.padding(bottom = 8.dp)) {
        when (f.type) {
            "bool" -> CardBox { SwitchRow(label, value == "1") { onEdit(if (it) "1" else "0") } }
            "sel" -> {
                var open by remember { mutableStateOf(false) }
                ExposedDropdownMenuBox(open, { open = it }) {
                    OutlinedTextField(
                        f.options.firstOrNull { it.first == value }?.second ?: value, {}, readOnly = true,
                        label = { Text(label) },
                        trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(open) },
                        modifier = Modifier.menuAnchor().fillMaxWidth()
                    )
                    ExposedDropdownMenu(open, { open = false }) {
                        f.options.forEach { (v, l) -> DropdownMenuItem({ Text(l) }, { onEdit(v); open = false }) }
                    }
                }
            }
            "pass" -> OutlinedTextField(
                value, onEdit, label = { Text(label) }, singleLine = true,
                placeholder = { Text(if (f.isSet) "•••••• (unchanged)" else "not set") },
                visualTransformation = PasswordVisualTransformation(),
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
                modifier = Modifier.fillMaxWidth()
            )
            else -> OutlinedTextField(
                value, onEdit, label = { Text(label) }, singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = if (f.type == "int" || f.type == "float") KeyboardType.Decimal else KeyboardType.Text),
                modifier = Modifier.fillMaxWidth()
            )
        }
    }
}
