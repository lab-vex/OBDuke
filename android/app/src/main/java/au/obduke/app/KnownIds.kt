package au.obduke.app

import org.json.JSONArray
import org.json.JSONObject
import java.math.BigDecimal
import java.math.MathContext
import java.util.Locale

/**
 * Known KTM-family CAN broadcast IDs, used to label the bus sniffer.
 *
 * The table lives in assets/can_ids.json (edit that, then run tools/gen_can_ids.py, which also
 * regenerates the copy the web page uses). Every layout in it comes from OTHER bikes, so the sniffer
 * shows each one as a hint with a value to check, never as a fact about this bike.
 */

/** One piece of a known frame: where it sits and how to read it. A field with [text] is only a note. */
data class KField(
    val name: String,
    val text: String?,
    val b: Int,
    val n: Int,
    val shift: Int,
    val bits: Int,
    val signed: Boolean,
    val le: Boolean,
    val mul: Double,
    val add: Double,
    val dp: Int,
    val unit: String,
    val min: Double?,
    val max: Double?,
    val map: Map<Long, String>,
    val yes: String?,
    val no: String?,
    val bikes: Map<String, String>,
    val note: String
)

data class KEntry(
    val idHex: String,
    val from: Int,
    val to: Int,
    val name: String,
    val ms: Int,
    val origin: String,
    val tryIt: String,
    val note: String,
    val bikes: Map<String, String>,
    val fields: List<KField>
)

/** A decoded value. [ok] is false when it falls outside what is believable, so the layout probably isn't this bike's. */
data class KValue(val text: String, val ok: Boolean)

class KnownIds(val bikeNames: Map<String, String>, private val entries: List<KEntry>) {

    val size get() = entries.size

    /** The entry for a standard 11-bit ID given as hex (e.g. "120"), or null. Extended IDs never match. */
    fun lookup(idHex: String): KEntry? {
        if (idHex.isEmpty() || idHex.length > 3) return null
        val v = idHex.toIntOrNull(16) ?: return null
        return entries.firstOrNull { v in it.from..it.to }
    }

    /** True for the diagnostic range (7xx) that the sniffer hides by default. */
    fun isDiag(idHex: String) = idHex.length == 3 && idHex.startsWith("7")

    /** Value of one field in [d] (byte values), or null if the frame is too short or the field is only a note. */
    fun value(f: KField, d: IntArray): KValue? {
        if (f.text != null || f.b + f.n > d.size) return null
        var r = 0L
        if (f.le) for (i in f.n - 1 downTo 0) r = r * 256 + d[f.b + i]
        else for (i in 0 until f.n) r = r * 256 + d[f.b + i]
        r = (r shr f.shift) and ((1L shl f.bits) - 1)
        if (f.signed && r >= (1L shl (f.bits - 1))) r -= 1L shl f.bits
        if (f.map.isNotEmpty()) {
            val m = f.map[r]
            return KValue(m ?: r.toString(), m != null || (f.min != null && f.max != null && r >= f.min && r <= f.max))
        }
        if (f.yes != null) return KValue(if (r != 0L) f.yes else (f.no ?: ""), true)
        val v = r * f.mul + f.add
        val t = String.format(Locale.ROOT, "%.${f.dp}f", v) + if (f.unit.isNotEmpty()) " ${f.unit}" else ""
        return KValue(t, f.min == null || f.max == null || (v >= f.min && v <= f.max))
    }

    /** A ready-to-paste Settings > Custom signals line for a field that sits on whole bytes, else "". */
    fun signalLine(e: KEntry, f: KField): String {
        if (f.text != null || f.map.isNotEmpty() || f.yes != null || f.shift != 0 || f.bits != 8 * f.n || f.n > 4) return ""
        val endian = if (f.le) (if (f.signed) "L" else "l") else (if (f.signed) "B" else "b")
        val name = f.name.replace(Regex("[:|]"), " ").take(28)
        val unit = f.unit.replace("?", "").replace(":", "").let { if (it == "raw") "" else it }
        return "${e.idHex}:${f.b}:${f.n}:$endian:${num(f.mul)}:${num(f.add)}:$name:$unit"
    }

    private fun num(x: Double): String =
        if (x == 0.0) "0" else BigDecimal(x).round(MathContext(4)).stripTrailingZeros().toPlainString()

    companion object {
        val EMPTY = KnownIds(emptyMap(), emptyList())

        fun parse(json: String): KnownIds {
            val root = JSONObject(json)
            val bikes = HashMap<String, String>()
            root.optJSONObject("bikes")?.let { o -> o.keys().forEach { bikes[it] = o.getString(it) } }
            val out = ArrayList<KEntry>()
            val ids = root.getJSONArray("ids")
            for (i in 0 until ids.length()) {
                val e = ids.getJSONObject(i)
                val from = e.getString("id").toInt(16)
                val to = if (e.has("to")) e.getString("to").toInt(16) else from
                val fields = ArrayList<KField>()
                val fa = e.optJSONArray("fields") ?: JSONArray()
                for (j in 0 until fa.length()) fields.add(field(fa.getJSONObject(j)))
                out.add(
                    KEntry(
                        e.getString("id"), from, to, e.getString("name"), e.optInt("ms"), e.optString("from"),
                        e.optString("try"), e.optString("note"), strMap(e.optJSONObject("bikes")), fields
                    )
                )
            }
            return KnownIds(bikes, out)
        }

        private fun strMap(o: JSONObject?): Map<String, String> {
            val m = LinkedHashMap<String, String>()
            if (o != null) o.keys().forEach { m[it] = o.getString(it) }
            return m
        }

        private fun field(o: JSONObject): KField {
            val n = o.optInt("n", 1)
            val map = LinkedHashMap<Long, String>()
            o.optJSONObject("map")?.let { mo -> mo.keys().forEach { map[it.toLong()] = mo.getString(it) } }
            return KField(
                name = o.optString("name"),
                text = if (o.has("text")) o.getString("text") else null,
                b = o.optInt("b"), n = n, shift = o.optInt("shift", 0), bits = o.optInt("bits", 8 * n),
                signed = o.optBoolean("signed"), le = o.optBoolean("le"),
                mul = o.optDouble("mul", 1.0), add = o.optDouble("add", 0.0), dp = o.optInt("dp", 0),
                unit = o.optString("unit"),
                min = if (o.has("min")) o.getDouble("min") else null,
                max = if (o.has("max")) o.getDouble("max") else null,
                map = map,
                yes = if (o.has("yes")) o.getString("yes") else null,
                no = if (o.has("no")) o.getString("no") else null,
                bikes = strMap(o.optJSONObject("bikes")),
                note = o.optString("note")
            )
        }
    }
}
