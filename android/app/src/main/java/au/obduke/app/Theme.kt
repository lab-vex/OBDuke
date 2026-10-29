package au.obduke.app

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.foundation.background
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.border
import androidx.compose.ui.draw.clip

object C {
    val Orange = Color(0xFFFF6600)
    val Orange2 = Color(0xFFFF8A3D)
    val Bg = Color(0xFF0A0A0A)
    val Card = Color(0xFF151515)
    val Card2 = Color(0xFF1D1D1D)
    val Line = Color(0xFF2B2B2B)
    val Text = Color(0xFFF2F2F2)
    val Muted = Color(0xFF9A9A9A)
    val Bad = Color(0xFFFF3B30)
    val Ok = Color(0xFF30D158)
    val Warn = Color(0xFFFFCC00)
}

private val scheme = darkColorScheme(
    primary = C.Orange, onPrimary = Color.Black,
    secondary = C.Orange2, onSecondary = Color.Black,
    background = C.Bg, onBackground = C.Text,
    surface = C.Card, onSurface = C.Text,
    surfaceVariant = C.Card2, onSurfaceVariant = C.Muted,
    outline = C.Line, error = C.Bad
)

@Composable
fun ObdukeTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = scheme, content = content)
}

/** Bold italic upper-case heading in the KTM style */
@Composable
fun Heading(text: String, modifier: Modifier = Modifier) {
    Text(
        text.uppercase(), modifier = modifier.padding(top = 18.dp, bottom = 10.dp),
        style = TextStyle(
            color = C.Orange, fontSize = 14.sp, fontWeight = FontWeight.ExtraBold,
            fontStyle = FontStyle.Italic, letterSpacing = 1.sp
        )
    )
}

@Composable
fun CardBox(modifier: Modifier = Modifier, borderColor: Color = C.Line, content: @Composable ColumnScope.() -> Unit) {
    Column(
        modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(C.Card)
            .border(1.dp, borderColor, RoundedCornerShape(12.dp))
            .padding(14.dp),
        content = content
    )
}

@Composable
fun Logo() {
    Row(verticalAlignment = androidx.compose.ui.Alignment.CenterVertically) {
        Box(
            Modifier
                .size(width = 12.dp, height = 26.dp)
                .rotate(20f)
                .background(C.Orange)
        )
        Spacer(Modifier.width(12.dp))
        Text(
            "OB", color = C.Text, fontSize = 24.sp,
            fontWeight = FontWeight.Black, fontStyle = FontStyle.Italic, letterSpacing = 1.sp
        )
        Text(
            "DUKE", color = C.Orange, fontSize = 24.sp,
            fontWeight = FontWeight.Black, fontStyle = FontStyle.Italic, letterSpacing = 1.sp
        )
    }
}

@Composable
fun KButton(
    text: String, onClick: () -> Unit, modifier: Modifier = Modifier,
    primary: Boolean = false, danger: Boolean = false, enabled: Boolean = true
) {
    val bg = when { danger -> C.Bad; primary -> C.Orange; else -> C.Card2 }
    val fg = when { danger -> Color.White; primary -> Color.Black; else -> C.Text }
    Button(
        onClick = onClick, enabled = enabled, modifier = modifier,
        shape = RoundedCornerShape(10.dp),
        colors = ButtonDefaults.buttonColors(containerColor = bg, contentColor = fg),
        border = if (!primary && !danger) androidx.compose.foundation.BorderStroke(1.dp, C.Line) else null,
        contentPadding = PaddingValues(horizontal = 16.dp, vertical = 12.dp)
    ) {
        Text(text.uppercase(), fontWeight = FontWeight.ExtraBold, fontStyle = FontStyle.Italic, letterSpacing = 0.5.sp)
    }
}
