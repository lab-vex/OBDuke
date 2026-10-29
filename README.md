# OBDuke

DIY diagnostic tool for the Gen 1 KTM 390 Duke (2013–2016), built on an ESP32-WROOM-32E with an SN65HVD230 CAN transceiver.

## Download

| | |
|---|---|
| 📱 **Android app** | [**obduke-app.apk**](https://github.com/lab-vex/obduke/releases/latest/download/obduke-app.apk) |
| 🔧 **Firmware (fresh flash)** | [**obduke-firmware-full.bin**](https://github.com/lab-vex/obduke/releases/latest/download/obduke-firmware-full.bin), flash at `0x0` |
| 🔁 **Firmware (update)** | [**obduke-firmware.bin**](https://github.com/lab-vex/obduke/releases/latest/download/obduke-firmware.bin), flash at `0x10000` over an existing install |

These links always point to the [latest release](https://github.com/lab-vex/obduke/releases/latest). On your phone, tap the APK link and allow your browser to install unknown apps. After that, the app updates itself.

One unit, three ways in:

| Link | Used by | How |
|---|---|---|
| Wi-Fi hotspot | Web page, and the OBDuke Android app if you pick Wi-Fi | Join `OBDuke`, open `1.1.1.1` |
| Bluetooth Classic | The OBDuke Android app, Torque Pro (or any ELM327 app), or the Teltonika FMC225 / FMC130 | Pair once with PIN 1234. One device connected at a time |
| USB serial | Laptop | 115200 baud, same text commands |

The ESP32-WROOM-32E doesn't have enough RAM to run Wi-Fi, Bluetooth Classic and BLE together, so everything Bluetooth goes over one Bluetooth Classic serial link. The OBDuke app's commands start with `!` and get JSON back; everything else is treated as ELM327, so Torque and the tracker work as before.

**New in app 1.3:** demo mode. No unit handy? Tap **Try demo mode** on the Connect screen for a pretend bike with live data, stored codes, modules, sniffer and logging, so you can try every screen.

**New in 1.2:** the Android app connects over Bluetooth (default) or Wi-Fi, with every web page feature on both.

**New in 1.1:** hotspot address 1.1.1.1, Torque Pro support, bus sniffer, ABS / other module codes, watch tool, gear indicator, custom signals (e.g. front/rear wheel speed once found), peak hold.

```
OBDuke/
  firmware/obduke/   obduke.ino, obduke_fw.cpp, web_ui.h  (open obduke.ino in Arduino IDE)
  android/           Android Studio project for the app
  test/              firmware tests against a simulated Duke (run test/run.sh on Linux)
  .github/workflows/ automatic builds and releases
```

## Build outputs (no Android Studio needed)

Every push to GitHub builds everything automatically:

- **Latest build:** repo **Actions** tab → newest *Build* run → **Artifacts** at the bottom: `obduke-app` (the APK) and `obduke-firmware`.
- **Releases:** bump `versionName` in `android/app/build.gradle.kts`, push, and wait for a green Build. Then publish a release on GitHub (**Releases → Draft a new release**, new tag `v<versionName>` on `main`). The APK and firmware are attached to the release automatically, and the download links above pick them up.

Firmware files:
- `obduke-firmware-full.bin`: flash at address `0x0` with any ESP flasher (e.g. the Espressif web flasher or `esptool.py write_flash 0x0 obduke-firmware-full.bin`). Includes the Huge APP partition table.
- `obduke-firmware.bin`: app only, for flashing over the top of an existing OBDuke install at `0x10000`.

Each push also runs the simulator tests in `test/`.

---

## 1. Flash the firmware

1. Arduino IDE 2.x with **esp32 by Espressif Systems** installed in Boards Manager (any 2.0.x or 3.x version).
2. Open `firmware/obduke/obduke.ino`. Its folder must also contain `obduke_fw.cpp` and `web_ui.h`.
3. **Tools** menu:
   - Board: **ESP32 Dev Module**
   - Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)**. This is required; the default partition is too small.
4. Upload. Hold **BOOT** if it sits on `Connecting...`.
5. In Serial Monitor at 115200 you should see `OBDuke 1.2.0`, `Hotspot OBDuke at 1.1.1.1`, the Bluetooth line, then `Ready`.

Updating from 1.0 or 1.1 keeps your saved settings; the new ones start at their defaults. No flash erase needed.

Wiring is unchanged from the test rig:

| SN65HVD230 | ESP32 |
|---|---|
| 3V3 | 3V3 |
| GND | GND (shared with the bike) |
| CTX | GPIO21 |
| CRX | GPIO22 |
| CANH / CANL | OBD2 pins 6 / 14 through the KTM 6-pin adapter |

The CAN pins can be changed in Settings if you rewire.

**Optional battery voltage:** wire 12 V → 47 kΩ → GPIO34 → 10 kΩ → GND. Then in Settings set *Battery sense pin* to `34` and *Battery divider ratio* to `5.7`. Measure with a meter and adjust the ratio until the reading matches. The Gen 1 ECU doesn't report battery voltage itself.

**Status LED** (GPIO2 on most dev boards):
- slow blink: no ECU, key off
- solid: ECU connected
- fast blink: FI light is on

---

## 2. Web page

1. Join Wi-Fi **OBDuke**. The default password is **obduke1234**. Change it in Settings.
2. The page should open by itself. If not, go to **http://1.1.1.1** (type the `http://`).

The address is set in Settings > Wi-Fi > *Hotspot address*. If 1.1.1.1 ever gives you trouble, it's usually one of these, and `10.1.1.1` avoids both:
- Phone **Private DNS** set to Cloudflare (`one.one.one.one`): switch it to *Automatic* or *Off* while on OBDuke.
- The phone sent the request over **mobile data**, so you get Cloudflare's own 1.1.1.1 page instead of OBDuke. Turn mobile data off for a moment, or tell Android to stay connected to OBDuke when it says the Wi-Fi has no internet.
3. To make it work like an app, use your browser's **Add to Home Screen**.

If you set up *Hotspot + home Wi-Fi* and enter your home network, it's also reachable at **http://obduke.local** (or the IP shown in Settings > About) while you're home.

---

## 3. Android app

1. Install **Android Studio** (free).
2. **File > Open**, then pick the `android` folder. Let Gradle sync; the first sync downloads a few hundred MB.
3. To get an APK: **Build > Build App Bundle(s) / APK(s) > Build APK(s)**. The file appears in `android/app/build/outputs/apk/`.
   You can also plug your phone in with USB debugging on and press **Run**.
4. Copy the APK to your phone and open it. Allow "install unknown apps" for your file manager when asked.

The app has the same five tabs as the web page (Dash, Codes, Tools, Log, Settings) and every feature: live data, gear, custom signals, peaks, codes, freeze frame, ABS / other modules, sniffer, watch, gear calibration, all the info tools, logging with CSV export, and all settings.

**Bluetooth (default, needs unit firmware 1.2):**
1. Phone **Settings > Bluetooth**: pair with **OBDuke**, PIN **1234**. Or tap **Pair new** on the app's Connect screen.
2. Open the app, allow the Bluetooth permission, pick OBDuke in the list and tap **Connect**. It reconnects automatically next time.
3. The unit takes one Bluetooth connection at a time. If Torque or the tracker is connected, the app can't be (and vice versa). Close Torque first; if the tracker is in range and grabs the link, turn off *Bluetooth ELM327 link* on the tracker side or connect over Wi-Fi instead.

The web login (if set) only protects Wi-Fi; on Bluetooth the pairing PIN is the lock.

**Wi-Fi:** tap **Wi-Fi** on the Connect screen. The app defaults to 1.1.1.1. If the unit answers on another address (e.g. it still has 1.0 firmware on 192.168.4.1), the app finds it through the hotspot's gateway and remembers it.

On Wi-Fi, check the hotspot name and password on the Connect screen (defaults `OBDuke` / `obduke1234`) and tap **Connect**. On Android 10+ the phone pops up a prompt to join the OBDuke Wi-Fi; allow it.

The app only sends its own traffic through the OBDuke hotspot, so mobile data keeps working for everything else. If you've already joined OBDuke in your phone's Wi-Fi settings, the app just uses that.

If you set a web login on the unit, the app asks for it once and remembers it.

Requires Android 8 or newer.

**Updates:** on launch the app checks this repo's latest GitHub Release and offers to update if it's newer (turn this off, or tap *Check now*, in **App updates** on the Connect screen or the Settings tab). It downloads `obduke-app.apk` from the release and opens Android's installer; the first time, Android asks you to allow OBDuke to install apps.

- If the repo is ever made private, the app needs a fine-grained GitHub token with **Contents: Read-only** on this repo, pasted into *App updates > GitHub token*.
- To publish an update: bump `versionName` in `android/app/build.gradle.kts`, push, wait for a green Build, then publish a GitHub Release with tag `v<versionName>`. The release build refuses a tag that doesn't match `versionName`.
- Release builds are signed with a permanent key kept in the repo secrets `OBDUKE_KEYSTORE_B64` and `OBDUKE_KEYSTORE_PASSWORD` (keep your own backup of it: if it's lost, every install has to be uninstalled once). A release tag fails to build if those secrets are missing. Anything installed before that (1.2.0 or older) was signed with a throwaway key, so uninstall it once and install 1.2.1 from Releases; from then on it updates itself.

Password fields have an eye button to show what you've typed.

---

## 4. Torque Pro (Bluetooth)

OBDuke shows up as an ELM327 adapter. Only one Bluetooth device can be connected at a time, so it's Torque **or** the tracker.

1. Phone **Settings > Bluetooth**: pair with **OBDuke**, PIN **1234** (or your *Bluetooth pairing PIN*).
2. Torque: **Settings > OBD2 Adapter Settings**: connection type **Bluetooth**, device **OBDuke**. Protocol can stay on *Automatic*. If it's slow, pick *ISO 15765-4 CAN (11 bit ID, 500 Kbaud)*.
3. Key on. Torque's realtime info should show *Connected to ECU*.

Works straight away: RPM (OBDuke answers 010C from KTM's 2121), speed, coolant, intake air, throttle, MAP, advance, fuel trims, O2, codes, clear codes. Battery voltage (ATRV / PID 42) is real only once the sense divider is wired; until then ATRV says 12.0 V.

**Extra PIDs for Torque** (Settings > Manage extra PIDs/Sensors > Add custom PID, OBD header blank or `7E0`):

| Name | PID | Equation | Notes |
|---|---|---|---|
| Gear | `22F010` | `A` | 0 = neutral / clutch in |
| Custom signal 1-4 | `22F001`-`22F004` | `((SIGNED(A)*256)+B)/10` | Whatever you set up in Custom signals |
| KTM RPM | `2121` | `((A*256)+B)/4` | Direct, same as the dash |
| Any KTM local ID | `21xx` | from the Watch tool | e.g. `2122`, byte positions as shown in Watch (subtract 2: Torque's A is the first byte after `61 xx`) |

For the ABS or another module, set the custom PID's header to that module's request ID from *Find modules* (e.g. `7B0`).

---

## 5. Teltonika tracker

The ESP shows up to the tracker as an ELM327 OBD dongle over Bluetooth Classic.

1. In OBDuke Settings > About, note the **Bluetooth MAC**.
2. In **Teltonika Configurator** for your FMC225 / FMC130:
   - **Bluetooth** tab: enable Bluetooth, set *BT Radio* to enabled, and set *Authentication* to PIN with **1234**, or whatever you set as *Tracker pairing PIN*.
   - **OBD II (Bluetooth)** tab: enable it and set the **External Name** to `OBDuke`, or enter the MAC from step 1.
   - Pick the OBD parameters you want sent to Traccar. The ones the Duke actually supports are coolant, intake temp, throttle, MAP, speed, ignition advance, fuel trims and O2.
3. Save to the device. Within a minute Settings > About in OBDuke should show **Tracker: Connected**.

**Virtual PIDs** (on by default): the Gen 1 ECU doesn't answer standard RPM (PID 0C) or battery voltage (PID 42). OBDuke fills these in itself, so the tracker gets them anyway:
- **RPM** comes from KTM's own 2121 request.
- **Battery voltage** comes from the sense pin, if you wired it.

**If pairing fails:** on ESP32 core 3.x, Secure Simple Pairing can override the PIN. Try setting the tracker to *no PIN*, or flash with ESP32 core 2.0.17.

---

## Settings

All settings are editable from the web page, the app, or serial (`set <key> <value>`). They're stored on the ESP and survive power cycles. Settings marked ↻ need a reboot.

| Group | Settings |
|---|---|
| Wi-Fi | Mode (hotspot / hotspot + home / off), hotspot name, password, address (default 1.1.1.1), channel, hidden, home Wi-Fi name + password, `.local` network name |
| Web page | Login user + password (blank = no login), auto-open page on join |
| Bluetooth ELM327 | Bluetooth name, link on/off, pairing PIN, virtual PIDs |
| CAN / ECU | Speed (auto / 500k / 250k), TX/RX pins, KTM RPM divisor, allow raw commands |
| Display | Metric / imperial, live refresh rate, coolant warning, RPM warning |
| Gear indicator | rpm per km/h for 1st-6th (learn them in Tools) |
| Custom signals | Up to 4 values decoded from bus frames or KTM local IDs |
| Battery | Sense pin, divider ratio, low-voltage warning |
| Logging | Interval, start on power-up |
| System | Status LED pin |

**Factory reset:** use Settings > Factory reset, or send `factory yes` over serial.

---

## What it does

- **Dashboard:** RPM (via KTM 2121), throttle, speed, coolant, intake air, MAP, ignition advance, short/long fuel trims, O2 voltage, battery, gear, plus your custom signals. Max RPM / speed / coolant are held until *Reset peaks*. Tiles turn red past your warning limits.
- **Codes:**
  - FI light status, number of codes, km with the light on, km since last clear
  - stored and pending codes, with descriptions
  - freeze frame
  - clear codes, with a confirmation step
  - **ABS & other modules:** *Find modules* pings every diagnostic address (key on, engine off, about 10 s) and lists who answers, then read / clear each one's codes (tries KWP2000, UDS and OBD in turn)
- **Tools:**
  - **bus sniffer**: live table of everything the bike broadcasts, with changing bytes highlighted (see below)
  - **watch a request**: reads one ECU request over and over and highlights bytes that move (read-only requests only)
  - **gear calibration**: ride steady in a gear and tap its number
  - supported PIDs, VIN, ECU identity
  - service check (which diagnostic services the ECU accepts)
  - KTM extended reads (2121/22/23/33) and a full local-ID scan
  - 3-second bus monitor
  - raw command (off by default)
- **Log:** records every live value into memory (600 samples, 10 minutes at 1 s) and exports as CSV.

Everything is read-only except **Clear codes** and the **raw command**, which you have to switch on in Settings. Nothing actuates parts of the bike.

## Finding wheel speeds (and other hidden values)

The ECU only reports one speed (PID 0D). Front and rear wheel speeds come from the Bosch ABS unit, and KTM's CAN IDs for them aren't published, so you find them yourself:

1. Bike on a paddock stand, key on, engine off. Tools > **Bus sniffer > Start**.
2. Tick *Only IDs changed since Mark*, tap **Mark**, then spin the **front wheel** by hand.
3. The ID whose bytes light up (and whose range grows as you spin faster) is your front wheel speed. Do the same for the rear.
4. Note the ID, the first byte and how many bytes (usually 2). Add it in Settings > **Custom signals**:

   `SOURCE:byte:length:b/l:scale:offset:name:unit`

   e.g. `120:2:2:b:0.01:0:Front wheel:km/h`
   - SOURCE: the CAN ID in hex, or a KTM local ID like `2122` (byte 0 = first byte after `61 22`)
   - b / l = big / little endian, B / L = signed
   - get the scale by comparing against the dash speed or GPS at a steady speed

The new tile appears on the Dash, in the log CSV and to Torque as `22F001`-`22F004`. The same trick works for brake switch, clutch, side stand and gear, if they're broadcast. Use **Watch** with `21 22` (etc.) to find values hidden in the KTM local IDs; rev the engine or flick a switch and watch which byte moves.

**Notes:**
- A full local-ID scan takes about a minute, and the unit doesn't answer anything else during it.
- Wi-Fi and the tracker's Bluetooth share one radio, so live data can be a little slower while the tracker is connected.

---

## Serial / app command reference

```
status            device + ECU status
codes             FI light, counters, stored + pending codes
clear yes         clear codes and FI light
freeze            freeze frame
live on|off       stream live data
pids | vin | ident | ktm | scan | discover
monitor <ms>      listen to the bus (500-10000 ms)
sniff start|mark|clear|stop   live bus sniffer; "sniff" reads the table
modules           find ECUs / ABS on the bus
mcodes <id>       read a module's codes, e.g. mcodes 7B0
mclear <id> yes   clear a module's codes
peek [@id] <hex>  read-only request (01 02 09 1A 21 22), e.g. peek @7B0 21 05
gear | gear learn <1-6> | gear reset
peak reset        reset max RPM / speed / coolant
raw <hex>         send raw request (needs allow_raw = 1)
settings          list all settings
set <key> <val>   change a setting
log start|stop|clear|status
logrows <start> <n>   log as CSV in chunks (what the app uses over Bluetooth)

Over Bluetooth, send any of these as "!<command>\r" (e.g. "!codes"); the reply is one JSON line.
reboot | factory yes
```
