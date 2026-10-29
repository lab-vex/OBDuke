package au.obduke.app

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontStyle
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.sin

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
        Image(
            painterResource(R.drawable.obduke_icon), contentDescription = "OBDuke",
            modifier = Modifier.align(Alignment.CenterHorizontally).padding(top = 4.dp, bottom = 16.dp)
                .size(132.dp).clip(RoundedCornerShape(28.dp))
        )
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
            Spacer(Modifier.height(8.dp))
            KButton("Try demo mode", { vm.connectDemo() }, enabled = st == LinkState.IDLE, modifier = Modifier.fillMaxWidth())
            Text(
                "No unit handy? Demo mode plays a pretend ride with some stored codes, so you can try every screen.",
                color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp)
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
                PasswordField(vm.wifiPass.value, { vm.wifiPass.value = it }, "Wi-Fi password", Modifier.fillMaxWidth())
                TextButton({ showAdv = !showAdv }) { Text(if (showAdv) "Hide advanced" else "Advanced", color = C.Orange) }
                if (showAdv) {
                    OutlinedTextField(vm.host.value, { vm.host.value = it }, label = { Text("Address") }, singleLine = true, modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(8.dp))
                    OutlinedTextField(vm.webUser.value, { vm.webUser.value = it }, label = { Text("Web login user") }, singleLine = true, modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(8.dp))
                    PasswordField(vm.webPass.value, { vm.webPass.value = it }, "Web login password (if set)", Modifier.fillMaxWidth())
                    Text("Address is 1.1.1.1 on the hotspot (192.168.4.1 on firmware 1.0 — the app finds either). On home Wi-Fi use the IP shown in the unit's Settings > About.",
                        color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
                }
            }
        }
        UpdatesCard(vm)
        Spacer(Modifier.height(24.dp))
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

// Tiles under the tacho, most-used first. RPM and gear live in the tacho itself.
private val GAUGES = listOf(
    Gauge("tps", "Throttle position", "%", 100.0, 0),
    Gauge("spd", "Speed", "km/h", 180.0, 0, imperial = { it * 0.6214 }, impUnit = "mph"),
    Gauge("ect", "Coolant", "°C", 120.0, 0, imperial = { it * 1.8 + 32 }, impUnit = "°F"),
    Gauge("vbat", "Battery", "V", 15.0, 2),
    Gauge("iat", "Intake", "°C", 80.0, 0, imperial = { it * 1.8 + 32 }, impUnit = "°F"),
    Gauge("map", "MAP", "kPa", 110.0, 0),
    Gauge("adv", "Advance", "°", 50.0, 1),
    Gauge("stft", "Short trim", "%", 25.0, 1, absBar = true),
    Gauge("ltft", "Long trim", "%", 25.0, 1, absBar = true),
    Gauge("o2", "O2", "V", 1.0, 2),
)

private const val TACHO_MAX = 12000.0

/** rpm past the shift warning where the shift light goes from flashing white to flashing red */
private const val SHIFT_RED_ABOVE = 250.0

/** Flips on/off every [periodMs] while it's non-null; steady on when null. */
@Composable
private fun rememberBlink(periodMs: Long?): Boolean {
    var on by remember { mutableStateOf(true) }
    LaunchedEffect(periodMs) {
        on = true
        if (periodMs != null) while (true) { kotlinx.coroutines.delay(periodMs); on = !on }
    }
    return on
}

@Composable
fun DashScreen(vm: ObdukeViewModel) {
    DisposableEffect(Unit) {
        vm.startLive()
        onDispose { vm.stopLive() }
    }
    val imp = vm.units.value == 1
    Column(Modifier.fillMaxSize().padding(horizontal = 12.dp)) {
        if (vm.fi.value == 1) {
            Spacer(Modifier.height(10.dp))
            CardBox(borderColor = C.Warn) {
                Text("FI LIGHT IS ON — ${vm.dtcCount.value} code(s). See Codes.", color = C.Warn, fontWeight = FontWeight.ExtraBold, fontSize = 13.sp)
            }
        } else if (!vm.ecu.value) {
            Text("Waiting for ECU — turn the key on.", color = C.Muted, fontSize = 13.sp, modifier = Modifier.padding(top = 10.dp))
        }
        LazyVerticalGrid(
            columns = GridCells.Fixed(3),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp),
            contentPadding = PaddingValues(vertical = 10.dp)
        ) {
            item(span = { GridItemSpan(3) }) {
                val rpm = vm.live["rpm"]
                TachoTile(rpm, vm.live["gear"], vm.live["spd"], vm.rpmWarn.value, vm.peaks["rpm"])
            }
            val all = GAUGES + vm.sigDefs.map { Gauge(it.key, it.name, it.unit, 0.0, 2) }
            items(all) { g ->
                val raw = vm.live[g.key]
                val warn = raw != null && when (g.key) {
                    "ect" -> raw >= vm.tempWarn.value
                    "vbat" -> raw < vm.vbatLow.value
                    else -> false
                }
                val shown = raw?.let { if (imp && g.imperial != null) g.imperial.invoke(it) else it }
                val unit = if (imp && g.imperial != null) g.impUnit else g.unit
                val pct = if (g.max <= 0) 0.0 else raw?.let { ((if (g.absBar) abs(it) else it) / g.max).coerceIn(0.0, 1.0) } ?: 0.0
                val pk = vm.peaks[g.key]?.let { p -> "max %.0f".format(if (imp && g.imperial != null) g.imperial.invoke(p) else p) }
                val text = if (shown == null) "--" else "%.${g.dp}f".format(shown)
                // overtemp / low battery pulse red, 500 ms on / 500 ms off
                val on = rememberBlink(if (warn) 500L else null)
                Tile(g.label, text, unit, pct.toFloat(), warn && on, raw == null, pk)
            }
            item(span = { GridItemSpan(3) }) {
                Row { KButton("Reset peaks", { vm.send("peak reset") }) }
            }
        }
    }
}

/** Tacho like the one on the app icon: 0-12k arc, red from the shift warning, gear in the middle. */
@Composable
private fun TachoTile(rpm: Double?, gear: Double?, spd: Double?, warnAt: Double, peak: Double?) {
    val frac = ((rpm ?: 0.0) / TACHO_MAX).toFloat().coerceIn(0f, 1f)
    val redFrac = (warnAt / TACHO_MAX).toFloat().coerceIn(0f, 1f)
    // Shift light: flashing white from the warning, flashing red from SHIFT_RED_ABOVE past it
    val stage = when {
        rpm == null || rpm < warnAt -> 0
        rpm < warnAt + SHIFT_RED_ABOVE -> 1
        else -> 2
    }
    val on = rememberBlink(when (stage) { 1 -> 350L; 2 -> 250L; else -> null })
    val lit = if (stage == 2) C.Bad else Color.White
    val warn = stage > 0
    val arcColor = when { !warn -> C.Orange; on -> lit; else -> Color(0xFF4A4A4A) }
    Row(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(14.dp))
            .background(C.Card)
            .border(if (warn && on) 2.dp else 1.dp, if (warn && on) lit else C.Line, RoundedCornerShape(14.dp))
            .padding(horizontal = 12.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(Modifier.size(128.dp), contentAlignment = Alignment.Center) {
            Canvas(Modifier.fillMaxSize()) {
                val sw = 9.dp.toPx()
                val inset = sw / 2 + 1.dp.toPx()
                val arc = Size(size.width - 2 * inset, size.height - 2 * inset)
                val tl = Offset(inset, inset)
                val start = 135f
                val sweep = 270f
                drawArc(C.Track, start, sweep, false, tl, arc, style = Stroke(sw))
                drawArc(C.Bad.copy(alpha = 0.35f), start + sweep * redFrac, sweep * (1 - redFrac), false, tl, arc, style = Stroke(sw))
                if (frac > 0f) drawArc(arcColor, start, sweep * frac, false, tl, arc, style = Stroke(sw, cap = StrokeCap.Round))
                // a tick every 1000 rpm, longer every 2000
                val r1 = arc.width / 2 - sw / 2 - 3.dp.toPx()
                for (i in 0..12) {
                    val a = Math.toRadians((start + sweep * i / 12f).toDouble())
                    val r2 = r1 - (if (i % 2 == 0) 8.dp else 4.dp).toPx()
                    val red = i * 1000.0 >= warnAt
                    drawLine(
                        if (red) C.Bad else Color(0xFFBDBDBD),
                        Offset(center.x + r1 * cos(a).toFloat(), center.y + r1 * sin(a).toFloat()),
                        Offset(center.x + r2 * cos(a).toFloat(), center.y + r2 * sin(a).toFloat()),
                        strokeWidth = 2.dp.toPx()
                    )
                }
            }
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                val g = gear
                Text(
                    when {
                        g == null -> "--"
                        g >= 0.5 -> "%.0f".format(g)
                        (spd ?: 0.0) < 8 -> "N"
                        else -> "–"
                    },
                    fontSize = 44.sp, fontWeight = FontWeight.Black, fontStyle = FontStyle.Italic,
                    color = if (g == null) Color(0xFF555555) else C.Orange
                )
                Text("GEAR", color = C.Muted, fontSize = 10.sp, fontWeight = FontWeight.Bold)
            }
        }
        Spacer(Modifier.width(14.dp))
        Column(Modifier.weight(1f)) {
            Text("RPM", color = C.Muted, fontSize = 11.sp, fontWeight = FontWeight.Bold)
            Text(
                rpm?.let { "%.0f".format(it) } ?: "--", fontSize = 40.sp, fontWeight = FontWeight.ExtraBold,
                fontStyle = FontStyle.Italic,
                color = when { rpm == null -> Color(0xFF555555); !warn -> C.Text; on -> lit; else -> Color(0xFF4A4A4A) }
            )
            Box(Modifier.fillMaxWidth().height(6.dp).clip(RoundedCornerShape(3.dp)).background(C.Track)) {
                Box(Modifier.fillMaxWidth(frac).fillMaxHeight().background(if (warn) androidx.compose.ui.graphics.SolidColor(arcColor) else C.Glow))
            }
            if (peak != null) Text(
                "max %.0f".format(peak), color = C.Muted, fontSize = 11.sp, fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(top = 6.dp)
            )
        }
    }
}

@Composable
private fun Tile(label: String, value: String, unit: String, pct: Float, warn: Boolean, na: Boolean, peak: String? = null) {
    Box(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(10.dp))
            .background(C.Card)
            .border(1.dp, if (warn) C.Bad else C.Line, RoundedCornerShape(10.dp))
    ) {
        Column(Modifier.padding(start = 10.dp, end = 8.dp, top = 8.dp, bottom = 10.dp)) {
            Text(
                label.uppercase(), color = C.Muted, fontWeight = FontWeight.Bold, maxLines = 1, softWrap = false,
                fontSize = if (label.length > 12) 9.sp else 10.sp, letterSpacing = if (label.length > 12) 0.sp else 0.5.sp
            )
            Row(verticalAlignment = Alignment.Bottom) {
                Text(
                    value, fontSize = 22.sp, fontWeight = FontWeight.ExtraBold, maxLines = 1,
                    color = when { na -> Color(0xFF555555); warn -> C.Bad; else -> C.Text }
                )
                Text(" $unit", color = C.Muted, fontSize = 11.sp, fontWeight = FontWeight.SemiBold, maxLines = 1, modifier = Modifier.padding(bottom = 3.dp))
            }
            Text(peak ?: " ", color = C.Muted, fontSize = 10.sp, fontWeight = FontWeight.Bold, maxLines = 1)
        }
        Box(
            Modifier.align(Alignment.BottomStart).fillMaxWidth(pct).height(3.dp)
                .background(if (warn) androidx.compose.ui.graphics.SolidColor(C.Bad) else C.Glow)
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
    val ctx = LocalContext.current
    val copy: (String, String) -> Unit = { text, msg ->
        try {
            (ctx.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager)
                .setPrimaryClip(ClipData.newPlainText("OBDuke", text))
            vm.toast.value = msg
        } catch (_: Exception) { vm.toast.value = "Couldn't copy" }
    }
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
        CheckRow("Only IDs not in the KTM list", vm.sniffOnlyUnknown.value) { vm.sniffOnlyUnknown.value = it; vm.sniffRedraw() }
        Text("Orange box = changed just now, orange text = changed since Mark. Tap Mark, spin the front wheel by hand and watch which bytes light up. A name under an ID means other KTMs use it: tap the name for the byte map, what to test, and a Custom signals line (tap that line to copy it). Those layouts come from other bikes, so a value marked with a cross probably isn't this bike's.",
            color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(vertical = 6.dp))
        MonoBox {
            if (vm.sniffSummary.value.isNotEmpty())
                Text(vm.sniffSummary.value, color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(bottom = 6.dp))
            if (vm.sniffMsg.value.isNotEmpty()) Text(vm.sniffMsg.value, color = C.Muted, fontSize = 12.sp)
            vm.sniffRows.forEach { SniffRowView(vm, it, copy) }
        }
        KButton("Copy capture report", {
            val t = vm.sniffReport()
            if (t.isEmpty()) vm.toast.value = "Nothing captured yet" else copy(t, "Capture copied")
        }, modifier = Modifier.fillMaxWidth().padding(top = 8.dp))

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
        if (!rawAllowed) Text("Turn on \"Allow raw ECU commands\" in Settings → Show advanced → CAN / ECU.", color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
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

/** A sniffer row, plus (if other KTMs use this ID) a tappable name that opens what each byte might mean. */
@Composable
private fun SniffRowView(vm: ObdukeViewModel, r: ByteRow, onCopy: (String, String) -> Unit) {
    ByteRowView(r)
    val k = r.known ?: return
    val open = vm.sniffOpen[r.id] == true
    Text(
        (if (open) "▾ " else "▸ ") + k.name, color = C.Orange2, fontSize = 12.sp,
        modifier = Modifier.fillMaxWidth().clickable { vm.sniffOpen[r.id] = !open }.padding(bottom = 4.dp)
    )
    if (open) KnownDetail(vm.known, k, r.data, onCopy)
}

@Composable
private fun KNote(t: String) = Text(t, color = C.Muted, fontSize = 11.sp)

@Composable
private fun KnownDetail(known: KnownIds, e: KEntry, d: IntArray, onCopy: (String, String) -> Unit) {
    Column(Modifier.fillMaxWidth().padding(start = 10.dp, bottom = 6.dp)) {
        if (e.bikes.isNotEmpty())
            KNote("Seen on: " + e.bikes.entries.joinToString(" · ") { "${known.bikeNames[it.key] ?: it.key} (${it.value})" })
        if (e.tryIt.isNotEmpty()) KNote("Test: ${e.tryIt}")
        if (e.note.isNotEmpty()) KNote(e.note)
        e.fields.forEach { f ->
            if (f.text != null) {
                KNote("D${f.b}  ${f.text}")
            } else {
                val v = known.value(f, d)
                val ok = v != null && v.ok
                Column(Modifier.fillMaxWidth().padding(top = 5.dp).alpha(if (ok) 1f else 0.6f)) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text(f.name, fontWeight = FontWeight.SemiBold, fontSize = 12.sp, modifier = Modifier.weight(1f))
                        Text(
                            if (f.n > 1) "D${f.b}-D${f.b + f.n - 1}" else "D${f.b}",
                            color = C.Muted, fontSize = 11.sp, fontFamily = FontFamily.Monospace
                        )
                        Spacer(Modifier.width(8.dp))
                        Text(
                            if (v == null) "-" else (if (v.ok) "" else "✗ ") + v.text,
                            color = if (ok) C.Ok else C.Warn, fontSize = 12.sp, fontFamily = FontFamily.Monospace
                        )
                    }
                    KNote(f.bikes.entries.joinToString(" · ") { "${it.key} ${it.value}" } + (if (f.note.isNotEmpty()) " - ${f.note}" else ""))
                    val sig = known.signalLine(e, f)
                    if (sig.isNotEmpty())
                        Text(
                            sig, color = C.Muted, fontSize = 11.sp, fontFamily = FontFamily.Monospace,
                            modifier = Modifier.clickable { onCopy(sig, "Custom signal line copied") }
                        )
                }
            }
        }
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
    var showAdv by remember { mutableStateOf(false) }
    var askBtOff by remember { mutableStateOf(false) }

    // The unit's settings, regrouped for the app: everyday ones up top, the rest under Advanced.
    val tidy = vm.fields.map { f ->
        f.copy(
            group = SETTING_GROUP[f.group] ?: f.group,
            label = SETTING_LABEL[f.key] ?: f.label
        )
    }
    val advKeys = ADVANCED.flatMap { it.second }.toSet()
    fun onEditFor(f: Field): (String) -> Unit = { v ->
        // switching Bluetooth off cuts the app (and Torque) off, so make sure first
        if (f.key == "trk_on" && v == "0" && (edits[f.key] ?: f.value) == "1") askBtOff = true
        else edits[f.key] = v
    }

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
        UpdatesCard(vm)

        tidy.filter { it.key !in advKeys }.groupBy { it.group }.forEach { (group, list) ->
            Heading(group)
            list.forEach { SettingField(it, edits[it.key], onEditFor(it)) }
        }
        if (vm.fields.isEmpty()) Text("Loading settings…", color = C.Muted, modifier = Modifier.padding(top = 12.dp))
        else {
            Spacer(Modifier.height(10.dp))
            KButton(if (showAdv) "Hide advanced" else "Show advanced", { showAdv = !showAdv }, modifier = Modifier.fillMaxWidth())
            if (showAdv) {
                Text(
                    "Home Wi-Fi, hotspot extras, Bluetooth on/off, CAN / ECU, gear ratios, log interval, custom signals and wiring. Most people never need these.",
                    color = C.Muted, fontSize = 12.sp, modifier = Modifier.padding(top = 8.dp)
                )
                ADVANCED.forEach { (title, keys) ->
                    val list = keys.mapNotNull { k -> tidy.firstOrNull { it.key == k } }
                    if (list.isNotEmpty()) {
                        Heading(title)
                        list.forEach { SettingField(it, edits[it.key], onEditFor(it)) }
                    }
                }
            }
        }
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
            KV("App", BuildConfig.VERSION_NAME)
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
    if (askBtOff) AlertDialog(
        onDismissRequest = { askBtOff = false }, containerColor = C.Card,
        title = { Text("Turn Bluetooth off?", fontWeight = FontWeight.ExtraBold) },
        text = {
            Text(
                "This switches the unit's Bluetooth off completely. The OBDuke app, Torque Pro and the tracker won't be able to connect over Bluetooth." +
                    (if (vm.transport.value == "bt" && !vm.demo.value) " You're connected over Bluetooth now, so the app will lose its connection when you save." else "") +
                    "\n\nTo turn it back on, join the OBDuke Wi-Fi and open the web page at ${vm.status.value.optString("ap_ip", "1.1.1.1")} (or use this app over Wi-Fi), then switch it on in Settings and reboot."
            )
        },
        confirmButton = { TextButton({ askBtOff = false; edits["trk_on"] = "0" }) { Text("TURN OFF", color = C.Bad, fontWeight = FontWeight.ExtraBold) } },
        dismissButton = { TextButton({ askBtOff = false }) { Text("CANCEL", color = C.Muted) } }
    )
    if (askFactory) AlertDialog(
        onDismissRequest = { askFactory = false }, containerColor = C.Card,
        title = { Text("Factory reset?", fontWeight = FontWeight.ExtraBold) },
        text = { Text("All settings go back to defaults and the unit reboots.") },
        confirmButton = { TextButton({ askFactory = false; vm.send("factory yes") }) { Text("RESET", color = C.Bad, fontWeight = FontWeight.ExtraBold) } },
        dismissButton = { TextButton({ askFactory = false }) { Text("CANCEL", color = C.Muted) } }
    )
}

/** The firmware's groups, renamed for the app. */
private val SETTING_GROUP = mapOf("Web page" to "Wi-Fi", "Bluetooth ELM327" to "Bluetooth")

private val SETTING_LABEL = mapOf("trk_on" to "Bluetooth (OBDuke app, Torque Pro, tracker)")

/** Settings shown only under Show advanced, by section. Anything not listed stays in the main list. */
private val ADVANCED = listOf(
    "Home Wi-Fi" to listOf("wifi_mode", "sta_ssid", "sta_pass", "hostname"),
    "Hotspot extras" to listOf("ap_ch", "ap_hidden", "captive"),
    "Bluetooth" to listOf("trk_on"),
    "CAN / ECU" to listOf("can_rate", "allow_raw", "rpm_div", "can_tx", "can_rx"),
    "Gear indicator" to listOf("gears"),
    "Logging" to listOf("log_ms"),
    "Custom signals" to listOf("sig1", "sig2", "sig3", "sig4"),
    "Hardware" to listOf("vbat_pin", "vbat_mult", "led_pin"),
)

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
            "pass" -> PasswordField(
                value, onEdit, label, Modifier.fillMaxWidth(),
                placeholder = if (f.isSet) "•••••• (unchanged)" else "not set"
            )
            else -> OutlinedTextField(
                value, onEdit, label = { Text(label) }, singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = if (f.type == "int" || f.type == "float") KeyboardType.Decimal else KeyboardType.Text),
                modifier = Modifier.fillMaxWidth()
            )
        }
    }
}
