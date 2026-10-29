# Known KTM-family CAN IDs

What other people have found on the CAN bus of KTM-family bikes, collected so the OBDuke bus sniffer can label what it hears. The sniffer's labels come from the same data as the tables at the bottom of this page (`android/app/src/main/assets/can_ids.json`).

## The honest bottom line

- **Nobody has published a broadcast capture from a 2013-16 (Gen 1) Duke 125/200/390.** Every layout below comes from other bikes: KTM 690 Enduro R, 1290 and 1390 Super Duke R, 390 Adventure (Gen 2) and Husqvarna 701 Enduro. The only IDs confirmed on a Gen 1 Duke are the diagnostic pair **0x7E0 / 0x7E8**.
- Gen 1 uses an older Bosch ECU (M17.8.x). It may broadcast none of these IDs, some of them, or the same IDs with different bytes. A sniffer capture on the bike is the only way to find out, and that is what the presets are for.
- The same ID can mean different things on different bikes (the kill switch is bit 4 of 0x120 D3 on the 690 but the start button on the 390 Adventure). So the sniffer shows every known variant next to the live bytes, and marks the ones whose value is believable.

## How the sniffer uses this

- Rows for IDs in the table get a name (orange). Tap a row to see every known field: where it sits in the frame, which bike it came from, how sure that source was, and the value it would give right now on your bike.
- A value that falls outside what is plausible (a coolant of 4000 °C, a gear of 9) is dimmed with a cross, so the layout that fits your bike stands out.
- Numeric fields that sit on whole bytes also show a ready-to-paste **Custom signals** line (Settings > Custom signals), so a match can become a dashboard tile.
- A summary line shows how many known IDs are on your bus and lists the ones that aren't known, and **Copy report** puts the whole capture on the clipboard so it can be shared.
- "Only IDs not in the KTM list" hides everything already explained, which leaves what is new to this bike.

How confident each source is:

| Status | Meaning |
| --- | --- |
| confirmed | checked against the dash or the bike's behaviour, by the project's owner or against a labelled capture log |
| capture | found in a real capture, not cross-checked (the unit or meaning may be a guess) |
| reported | stated in a forum post, no capture to check |
| candidate | educated guess from the byte pattern |

## Who published what

| Bike | Where | What it gave |
| --- | --- | --- |
| KTM 1290 Super Duke R (2017) | Dan Plastina's thread on ADVrider, quoted by ktm-can | The original IDs 0x120, 0x129, 0x12A, 0x12B, 0x290, 0x450, 0x540, and the lean-angle encoding. The thread itself would not load for me. |
| KTM 690 Enduro R (2020) | [blalor/ktm-can](https://github.com/blalor/ktm-can) and [ChickenPoptart/KTMComputer](https://github.com/ChickenPoptart/KTMComputer) | A tested decoder, plus a ride capture with markers for brakes, clutch, stand, TC button, map button and lean. Wheel speed scale from a ride. |
| KTM 390 Adventure Gen 2 (2026) | [jayasuryashaju/adv390-dash](https://github.com/jayasuryashaju/adv390-dash) | The most recent look at a 390-platform bike: 500 kbps, RPM confirmed at 0x120, wheel speed at 0x12D/0x12E, odometer, quickshifter, fuel flow. 0x12B is **not** sent. |
| Husqvarna 701 Enduro (Euro 5, 2023) | [jroal/701display](https://github.com/jroal/701display) | 0x12B front wheel speed fitted against OBD speed (17.77 counts per km/h), gear and side stand in 0x540, off-road ABS bit in 0x12A. Needs both the engine ECU and the ABS ECU awake. |
| KTM 1390 Super Duke R Evo (2024) | [sruon/ktm-1390-can](https://github.com/sruon/ktm-1390-can) | The best-labelled data so far: 17 candump logs, one per action (kill switch, clutch, each brake, slip adjuster, ride modes, suspension profiles, turn signals, high beam, buttons). I checked its README against the raw frames rather than trusting it, and it corrected earlier entries: lean angle (not rear brake) is in 0x12F, the clutch is in 0x129, the kill-switch and start-button bits match the 390 Adventure, and front and rear brake switches have separate bits in 0x290. Its README counts bits from the left (its B0 is 0x80); this page uses bit 0 = 0x01. It does not send 0x12B. Needs a bike with the newer switchgear and dash, so many of its IDs (0x140, 0x550, 0x5D1) may not exist on a Gen 1. |
| KTM 890 Duke R | [RaceChrono forum thread](https://racechrono.com/forum/discussion/comment/12434/) | Comments only, no capture. One (hedged "if I remember correctly") puts front and rear wheel speed at PID 299 (= 0x12B); another says 299 carries no wheel speed on the 890 R; a third guesses lean/tilt is PID 303 (= 0x12F) because the 890 may use the newer Super Duke ABS. RaceChrono PIDs are decimal. Weak evidence, but it agrees with the 1390 logs on 0x12F. |
| KTM 890 / 790 Duke | 790dukeforum.com threads, quoted by [ktm-890-vault](https://github.com/giacomoschito/ktm-890-vault) | Reports that throttle, gear/clutch and front brake sit in the same IDs as the 690. The threads would not load for me. |

The 690 Enduro R, Husqvarna 701 Enduro/Supermoto and GasGas ES/SM 700 share a platform and Bosch cornering ABS, so the 690 layouts are likely to carry across (KTMComputer lists this as "should work, untested").

## ECUs, ABS units and which bikes share them

| Family | Bikes | Source |
| --- | --- | --- |
| Bosch **M17.8.x** (M17.8.42 on the 125/200/390) | KTM Duke 125, 200, 390 (Gen 1). AltTune lists M17.8.7 for a Duke 200 ABS, and a 390-forum thread is titled for M17.8.7 on the 390. | [Alientech K-Suite 3.61 list](https://tuning-shop.com/about-us/news/alientech-tool-update-k-suite-release-361/), [AltTune](https://alttune.com/tuning/ktm-duke-a1643/) |
| Bosch **MSE2 SPC572** | KTM Duke 200 / 200 ABS / 390 / 390 ABS (newer generation), **Husqvarna Svartpilen 401**, Bajaj Dominar 400 | [ECUtools K-TAG update](https://ecutools.eu/publications/k-tag-update-30042021/) |
| Bosch **ME17.9.21** | KTM 790 Duke, KTM 790 Adventure | Alientech K-Suite 3.61 list |
| Gen 1 **ABS unit**, KTM part 90642031000 | Listed by used-parts sellers as fitting the 2013-16 125 and 390 Duke, the 2013-14 200 Duke and the RC 390 | [eBay listing](https://www.ebay.com/itm/253220395002) (seller claim) |

What I could not confirm: a Bosch model name for the Gen 1 ABS unit, which other brands' bikes use it, or the Pulsar NS200 / RS200 ECU family (an earlier search suggested the M17.8 family, but I could not re-check it). Public sources do not list ABS modules by bike well, and no CAN message list for any of these ABS units turned up. The Svartpilen 401 is listed on the newer MSE2 ECU, so it is closer to the Gen 2 Duke than to a Gen 1.

## Connectors and wire colours

| Bike | Pins | Source |
| --- | --- | --- |
| 2013-16 Duke (the pins OBDuke's lead uses) | 6-pin: 1 = +12 V, 2 = CAN-H, 3 = CAN-L, 6 = ground | This project |
| KTM RC 390 | KTM wire colours: white = battery, black/yellow = ground, blue = CAN-H, yellow = CAN-L (grey female connector under the seat) | [AiM RC390 template](https://www.aim-sportline.com/download/ecu/stock/ktm/KTM_RC390_100_eng.pdf) |
| 2020 Husqvarna Svartpilen 401 (Euro 5, ISO 19689) | 2 = CAN-H (green/brown), 3 = ground, 4 = power (grey/pink), 5 = CAN-L (yellow/brown). **Different from the Gen 1 Duke**, so a Gen 1 lead does not fit it. | [HQV Adventure forum](https://hqvadventure.com/forum/threads/obd2-on-401.375/) (confirmed by the poster against the repair manual) |
| 2019-20 690 Enduro R | White 6-pin connector near the battery. 2021+ Euro 5 bikes mostly moved to a red connector with different CAN pins. | KTMComputer |

## What the OBD-II side says

- AiM's RC 390 template ("390_CAN_OBD") lists RPM, GEAR, OBDII speed, coolant, intake air, MAP and throttle, so rpm and gear are obtainable from an RC 390's OBD/CAN stream. AiM does not say whether that is broadcast or request-based.
- Svartpilen 401 owners report O2 voltage and fuel trims work in Torque and OBDLink, but the AFR PIDs (0144 / 0134) do not.
- No source gave a list of broadcast IDs for the 2018-2023 Husqvarna 401s or for any Gen 1 bike.

## What I could and could not read

Read and used: the GitHub repos above (cloned and read; sruon/ktm-1390-can was also checked frame by frame against its 17 logs), the RaceChrono forum thread, the HQV Adventure forum threads, AiM's RC 390 PDF, ECUtools and Alientech ECU lists, eBay fitment listings.

Could not load (the sites refuse automated requests, and no browser was connected): the ADVrider 1290 CAN thread, the 790dukeforum threads (CAN mapping, 890R CAN data, 890R OBD data pull), the ktmduke390forum threads (CAN bus details of the 390, OBD reader, PIDs, Gen 1 connector), the rc390-forum Arduino logger and connector threads, and Bajaj forum threads. The archived copy of the superduke.net forum has the 1290 CAN thread, but its useful posts are marked as unrecoverable. Searches of Reddit, YouTube and other social media turned up nothing with decoded IDs, and GitHub code search was not available, so there may be small repos that were not found.

Also looked at and left out: EduRestrepo/KTM-Bus (its own header says the IDs are unverified guesses), and duke-dash, KTM-Nav-GEN3 and Connected-to-my-ktm (they talk Bluetooth to the dash and use no CAN IDs). Motoro is for Keihin K-line bikes, not CAN.

If you can open any of those threads and paste what they say, it can be added to `can_ids.json` in a couple of minutes.

## IDs

<!-- IDS-BEGIN -->
#### 0x120: Engine: rpm, throttle, switches (every 20 ms, from engine ECU)

Test: Key on, then rev the engine or twist the grip. Flip the kill switch. Press start.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0-D1 | Engine speed | big-endian; rpm | 690: confirmed, Adv390: confirmed, 1390: confirmed |
| D2 | Throttle grip (commanded) (raw 0 = closed, 253-255 = fully open. 1390 Evo: 0 at idle, 24-44 on the blips in the start-up log) | x 0.3953; % | 690: confirmed, Adv390: confirmed, 890: reported, 1390: capture |
| D3 | Kill switch | bit 4 (0x10); 1=RUN, 0=STOP | 690: confirmed |
| D3 | Kill switch (the same bit the 690 uses for the kill switch is the start button here. 1390 Evo: 08 with the switch on RUN, 00 on STOP) | bit 3 (0x08); 1=RUN, 0=STOP | Adv390: confirmed, 1390: confirmed |
| D3 | Start button (1390 Evo: D3 = 18 while cranking) | bit 4 (0x10); 1=PRESSED, 0=- | Adv390: confirmed, 1390: confirmed |
| D4 | Throttle map in use | bit 0 (0x01); values: 0=map 1, 1=map 2 | 690: confirmed |
| D4 | Ride mode (names assumed from the menu order) | values: 0=Street, 1=Offroad, 2=Rain | Adv390: capture |
| D7 | alive counter, steps by 0x20 (30, 50, 70, 90, B0, D0) | | |

Sources: [ktmcan](https://github.com/blalor/ktm-can), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [vault890](https://github.com/giacomoschito/ktm-890-vault), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x121: Throttle grip sensors (every 20 ms, from engine ECU)

Test: Twist the grip with the key on. Two bytes should rise together.  
Note: D1 and D3 rise with the grip: probably the two redundant ride-by-wire sensors. 1390 Evo: D5 tracked engine speed closely in the start-up log (correlation 0.8), so the layout may differ there.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D1 | Grip sensor A? | raw byte | 690: candidate |
| D3 | Grip sensor B? | raw byte | 690: candidate |
| D7 | rolling counter | | |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x128: Throttle closed switch (every 20 ms, from key-on)

Test: Key on: twist the grip off its stop and let go.  
Note: 690: reads 7F FF ... until the ECU wakes, then D2 toggles 02/03 with the engine running. 1390 Evo: D0-D1 stay 13 88 and D2 steps 01, 02, 03 through a start-up, so there it is a state, not a throttle switch (the repo README says gear and clutch live here, but its clutch log shows them in 129).  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D2 | Throttle | bit 0 (0x01); 1=closed, 0=open | Adv390: confirmed, 690: candidate |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x129: Gear and clutch (every 20 ms, from engine ECU)

Test: Key on, engine off: roll the bike and change gear. Pull the clutch lever.  
Note: 701 Enduro: the 701display firmware watches D0 bit 3 going LOW to spot the off-road ABS button being held while stopped. 1390 Evo: the clutch bit toggled in its lever-pull log; gear stayed 0 (neutral) in every log, so gear is unchecked there.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | Gear | 4 bits, shifted right 4; values: 0=N | 690: confirmed, Adv390: confirmed, 890: reported |
| D0 | Clutch | bit 3 (0x08); 1=PULLED, 0=released | 690: confirmed, Adv390: confirmed, 890: reported, 1390: confirmed |
| D7 | alive counter, steps by 0x20 (20, 40, 60, 80, A0, C0) | | |

Sources: [ktmcan](https://github.com/blalor/ktm-can), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [hq701](https://github.com/jroal/701display), [vault890](https://github.com/giacomoschito/ktm-890-vault), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x12A: Ride mode, traction and ABS states (every 50 ms, from key-on)

Test: Bike stopped: change ride mode, press the TC button, switch ABS mode.  
Note: The bit positions differ between models, so try each variant against what you change.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | Throttle | bit 1 (0x02); 1=closed, 0=open | 690: capture |
| D1 | Requested throttle map | bit 6 (0x40); values: 0=map 1, 1=map 2 | 690: confirmed |
| D1 | Traction control (D1 = 28 on, 18 off. Offroad ride mode also turns it off.) | bit 4 (0x10); 1=OFF, 0=ON | Adv390: confirmed |
| D2 | Off-road ABS mode | bit 0 (0x01); 1=OFF-ROAD, 0=road | 701: capture |
| D4 | Traction control (second bit) | bit 5 (0x20); 1=set, 0=clear | Adv390: capture |
| D4 | ABS mode (second bit) | bit 3 (0x08); 1=set, 0=clear | Adv390: capture |
| D6 | ABS mode (names not known) | values: 2=mode A, 4=mode B | Adv390: capture |
| D3 | Slip adjuster (MTC) level (1C = 7, 20 = 8, 24 = 9 while stepping the slip adjuster 7, 8, 9, 8, 7) | x 0.25 | 1390: confirmed |
| D2 | 1390 Evo: D2 high nibble is 7 in Performance and 1 in Rain (anti-wheelie level, per the repo). D4 moved with the ride mode (60, 44, 04, 24 seen). | | |

Sources: [ktmcan](https://github.com/blalor/ktm-can), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [hq701](https://github.com/jroal/701display), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x12B: ABS wheel speeds and lean (every 10 ms, from ABS)

Test: Bike on a stand, key on: spin the front wheel by hand, then the rear. Lean the bike on the stand for tilt and lean.  
Note: Not sent at all on the 390 Adventure (Gen 2) or the 1390 Evo (no 12B in any of its 17 logs); a RaceChrono commenter says 0x12B does not carry wheel speed on the 890 R. Scale: 701 Enduro fitted 17.77 counts per km/h against its OBD speed; the 690 speedometer gave about 17.0. Lean: 0 is upright, 1 and up lean right, FFF and down lean left (1290 thread). A 790 forum post (no ID given) says speed is (D0*256+D1)/16, unverified. One RaceChrono commenter (hedged with 'if I remember correctly') put front and rear wheel speed at PID 299 (= 0x12B) as bit ranges, calling them ABS pulse frequencies, which fits the layout here; another says 299 carries no wheel speed on the 890 R.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0-D1 | Front wheel speed | big-endian; x 0.05627; km/h | 690: confirmed, 701: confirmed |
| D2-D3 | Rear wheel speed (agrees with the front within about 3%) | big-endian; x 0.05627; km/h | 690: confirmed |
| D5-D6 | Tilt (raw) (12-bit signed, not validated) | 12 bits, shifted right 4; big-endian; signed | 1290: candidate, 690: candidate |
| D6-D7 | Lean angle (raw) (12-bit signed; degrees per count not published) | 12 bits; big-endian; signed | 1290: confirmed, 690: candidate |

Sources: [ktmcan](https://github.com/blalor/ktm-can), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [hq701](https://github.com/jroal/701display), [advrider](https://www.advrider.com/f/threads/results-from-hacking-the-ktm-superduke-1290-can-bus.1200087/), [racechrono](https://racechrono.com/forum/discussion/comment/12434/), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x12C: Slow wheel-speed copy? (every 1000 ms, from key-on)

Seen on: KTM 690 Enduro R, 2020 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | all zero when stopped; two 12-bit values follow wheel speed (slower, different scale) | | |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer)

#### 0x12D: Rear wheel speed (390 Adventure) (from ABS?)

Test: Bike on a stand: spin the rear wheel by hand. Compare with the dash on a ride.  
Note: Also sent on the 1390 Evo (about every 10 ms); with the wheels still only the counter in D7 changed.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D1-D2 | Rear wheel speed (a per-pulse count; the unit (0.1 km/h per count) is a guess, calibrate against the dash) | big-endian; x 0.1; km/h? | Adv390: capture |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x12E: Smoothed speed (390 Adventure) (from ABS?)

Note: 690: D0 is a status flag (00/40) and D7 a counter. Also sent on the 1390 Evo; with the bike still D6 and D7 changed there.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D1-D2 | Speed, smoothed (about 0.7 x the 12D value, fades out after the wheel stops) | big-endian; x 0.1; km/h? | Adv390: capture |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x12F: Lean angle (rear brake?) (every 10 ms, from key-on)

Test: Tilt the bike on its stand. Then press only the rear pedal. See which one moves D0-D1.  
Note: Evidence for lean: on the 1390 Evo D0-D1 sat at F83x (-125 counts) with the bike upright on its stand and ignored the rear pedal, and a RaceChrono commenter guesses lean/tilt is at PID 303 (= 0x12F) on the 890 R, which has the newer Super Duke ABS (a guess, no capture). The 390 Adventure project claims both uses for D0-D1 and has not settled it. D1 bit 0x20 also flips in bursts there (a status bit, not identified).  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0-D1 | Lean angle (raw) (tilted by hand on the stand: negative left, positive right. 1390 Evo upright on its stand: -125, jittering by 1-2 counts) | 12 bits, shifted right 4; big-endian; signed | Adv390: capture, 1390: capture |
| D0-D1 | Rear brake (raw) (rests near 304, rises 50-90 with the pedal (390 Adventure). The 1390 Evo's D0-D1 did not move with the pedal) | big-endian; raw | Adv390: capture |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can), [racechrono](https://racechrono.com/forum/discussion/comment/12434/)

#### 0x140: Handlebar buttons (1390 Evo) (every 20 ms, from switchgear)

Seen on: KTM 1390 Super Duke R Evo, 2024 (confirmed)  
Test: With the key on, use the turn-signal switch, the cancel button and the high-beam flash and watch which bit pulses.  
Note: Each button sets its own bit while held, and D2 rests at 03. The four below were checked against the repo's turn-signal and high-beam logs. The repo README counts bits from the left (its B0 is 0x80); everything here uses bit 0 = 0x01. Newer switchgear: whether a Gen 1 sends this is unknown.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | Left turn signal button | bit 4 (0x10); 1=PRESSED, 0=- | 1390: confirmed |
| D0 | Turn signal cancel button | bit 3 (0x08); 1=PRESSED, 0=- | 1390: confirmed |
| D0 | Right turn signal button | bit 2 (0x04); 1=PRESSED, 0=- | 1390: confirmed |
| D1 | High beam / flash button | bit 0 (0x01); 1=PRESSED, 0=- | 1390: confirmed |
| D0 | Other bits, from the repo README only (not checked one by one): D0 bit 1 = C1, bit 0 = C2; D1 bit 7 = cruise, bit 4 = +/resume, bit 3 = -/set, bit 1 = horn; D2 bit 7 right, 6 left, 5 up, 4 down, 3 centre of the d-pad. D2 bit 2 also pulses in the button log. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x174: Motion sensor (IMU)? (every 10 ms, from key-on)

Seen on: KTM 690 Enduro R, 2020 (candidate), KTM 1390 Super Duke R Evo, 2024 (candidate)  
Test: Lean the bike on the stand and watch which of 174, 178 and 17C swings.  
Note: Also on the 1390 Evo, where almost every byte jitters even with the bike upright on its stand.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | two 16-bit values centred near 0x8000 (one offset by gravity); counter in D6 low nibble; checksum in D7 | | |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x176: Motion or ABS data? (390 Adventure) (every 10 ms, from key-on)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0 and D4 jitter at rest; D6 low nibble counts 0-F; D7 looks like a checksum. Large erratic jumps = raw sensor data, not a clean speed. | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash)

#### 0x177: Motion or ABS data? (390 Adventure) (every 10 ms, from key-on)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D4 | D4 jitters at rest; D6 low nibble counts 0-F; D7 looks like a checksum. | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash)

#### 0x178: Motion sensor (IMU)? (every 10 ms, from key-on)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  
Test: Lean the bike on the stand: D0-D1 should swing.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0-D1 | Lean candidate (swings most when the bike leans) | big-endian; raw | 690: candidate |
| D6 | counter in D6 low nibble; checksum in D7 | | |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x17C: Motion sensor (IMU)? (every 10 ms, from key-on)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  
Test: Lean the bike on the stand: D4-D5 should swing.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D4-D5 | Lean candidate (swings most when the bike leans) | big-endian; raw | 690: candidate |
| D6 | counter in D6 low nibble; checksum in D7 | | |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x290: Brake pressure and brake light (every 10 ms, from ABS?)

Test: Key on: squeeze the front lever, then press the rear pedal.  
Note: 690: D2-D7 stay 0, so there is no brake-light bit there.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0-D1 | Front brake pressure (raw) (390 Adventure: rest noise up to about 48, firm stop 15,000-20,000. 1390 Evo: 0 at rest, about 8,500 at the peak of a hard squeeze on the stand) | big-endian; raw | 690: confirmed, Adv390: confirmed, 890: reported, 1390: confirmed |
| D3 | Front brake lever switch (D3 = 02 at rest, 0A with the lever) | bit 3 (0x08); 1=PRESSED, 0=- | 1390: confirmed |
| D3 | Rear brake pedal switch (D3 = 06 with the pedal. Rear pedal pressure did not show in D0-D1 (peak 32)) | bit 2 (0x04); 1=PRESSED, 0=- | 1390: confirmed |
| D3 | 390 Adventure: D3 goes 02 to 0E when the brake light is on (both bits at once), unlike the 1390 Evo where each switch has its own bit | | |

Sources: [ktmcan](https://github.com/blalor/ktm-can), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [vault890](https://github.com/giacomoschito/ktm-890-vault), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x300: Constant 0F... (off-road ABS dongle?) (every 200 ms, from key-on)

Seen on: KTM 690 Enduro R, 2020 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | always 0F... on the 690; maybe the off-road ABS dongle, untested with it unplugged | | |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer)

#### 0x310: Not decoded (1390 Evo) (every 100 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0-D3 are either 71 7A 4C 54 or FF FF FF FF (looks like four sensor values that read FF when invalid); D4-D5 stay 7D 7D. Meaning not known. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x400: Not decoded (1390 Evo) (every 10 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0 reads 28 at rest and swings while the engine cranks (down to 0B, then back up); D1 is 80, sometimes C0; D7 is a counter. Meaning not known. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x410: Not decoded (1390 Evo) (every 10 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0, D3 and D6 follow engine speed loosely (correlation about 0.7 in the start-up log); D1, D4 and D7 vary too. Meaning not known. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x450: Buttons and ABS mode (every 50 ms, from key-on)

Test: Press the TC button and the map button. Change ABS mode with the bike stopped.  
Note: 390 Adventure: D2 sits at 0x80 at rest, so the 690's TC-button bit may not apply. 1390 Evo: the top bits of D2 step through 20, 40, 60 ... between logs (a slow counter), and D4 changed (1B, 13, 0B, 03) in the logs where the ride mode was switched.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D2 | TC button | bit 0 (0x01); 1=PRESSED, 0=- | 690: confirmed |
| D4 | Map button / request | values: 0=map 0, 9=map 1 | 690: confirmed |
| D3 | ABS mode (names assumed) | values: 1=Road, 2=Offroad | Adv390: capture |
| D4 | Traction (second bit) | bit 3 (0x08); 1=set, 0=clear | Adv390: capture |

Sources: [ktmcan](https://github.com/blalor/ktm-can), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x4E8 to 0x4E9: Start-up exchange (1390 Evo) (from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  
Note: Only 12 frames in the whole set, right after key-on: 4E8 = 55 01 ..., 55 02 ..., 55 03 ... with random-looking bytes, 4E9 = 5A 01 01, 5A 02 01, 5A 03 01. Looks like a key or security exchange, so there is nothing to decode.  

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x4EA: Dashboard warning light? (every 100 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (capture)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D4 | Orange warning light (D4 = 01 in the log taken with the kill switch off, all zero in every other log; the repo README names it as the dashboard's orange light) | bit 0 (0x01); 1=ON, 0=off | 1390: capture |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x540: Slow status: coolant, gear, stand, fuel (every 100 ms, from engine ECU)

Test: Key on: raise and lower the side stand. Warm the engine and compare coolant with the dash.  
Note: The 690 and 390 Adventure put coolant and the side stand one byte apart, so the variants disagree. Whichever gives a believable number is the one for your bike. The 1390 Evo agrees with the 390 Adventure for coolant.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D1-D2 | Engine speed (slow copy) | big-endian; rpm | 690: confirmed |
| D3 | Gear (slow copy) | 4 bits; values: 0=N, 7=shifting | 690: confirmed, 701: capture |
| D4 | Side stand | bit 0 (0x01); 1=UP, 0=DOWN | 690: confirmed, 701: capture |
| D4 | Side stand fault | bit 7 (0x80); 1=FAULT, 0=ok | 690: capture |
| D6-D7 | Coolant | big-endian; x 0.1; °C | 690: confirmed |
| D3 | Side stand | bit 0 (0x01); 1=UP, 0=DOWN | Adv390: confirmed |
| D5-D6 | Coolant (96.2 after a ride (390 Adventure). 1390 Evo: 95.8 in a warm-engine log, 15.4 on a cold key-on, climbing through a start-up; check against the dash) | big-endian; x 0.1; °C | Adv390: capture, 1390: capture |
| D1-D2 | Fuel flow (raw per second) (0 off, about 27 just after start, 19-22 idling; litres per hour not calibrated) | little-endian; raw | Adv390: capture |
| D3 | 1390 Evo: D3 reads 10 before the engine starts and 00 once it is running (meaning not known). D1 follows engine speed loosely but is not a clean copy. | | |

Sources: [ktmcan](https://github.com/blalor/ktm-can), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [hq701](https://github.com/jroal/701display), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x541: Quickshifter and timers (from engine ECU)

Note: 690: always 02 80. 390 Adventure: a byte counts seconds since start (D4 or D6 in the two captures), so it is a timer, not a sensor.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | Quickshifter | bit 2 (0x04); 1=ON, 0=OFF | Adv390: confirmed |
| D4 | seconds-since-start style counter | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktmcomp](https://github.com/ChickenPoptart/KTMComputer)

#### 0x550: Lights and status flags (every 200 ms, from key-on)

Seen on: KTM 690 Enduro R, 2020 (candidate), KTM 1390 Super Duke R Evo, 2024 (confirmed)  
Test: Switch the indicators, the hazards and the high beam on and off with the key on.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | 690: D0 flips between 00 and 10; D7 is a counter | | |
| D2 | Left indicator lit (toggles about every 0.4 s while flashing; both lit = hazards) | bit 0 (0x01); 1=ON, 0=off | 1390: confirmed |
| D2 | Right indicator lit | bit 1 (0x02); 1=ON, 0=off | 1390: confirmed |
| D4 | High beam lit | bit 4 (0x10); 1=ON, 0=off | 1390: confirmed |
| D4 | Daytime running light | bit 0 (0x01); 1=ON, 0=off | 1390: confirmed |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x551: Static with engine off (390 Adventure) (from key-on)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate), KTM 1390 Super Duke R Evo, 2024 (candidate)  
Note: 1390 Evo: also static, 00 00 00 00 00 04 C0 00 in every log.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | static with the engine off; capture again riding | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x552: Not decoded (1390 Evo) (every 50 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0 and D7 vary (D7 looks like a counter); the other bytes stay put. Meaning not known. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x553: Slow static: trip or service data? (every 1000 ms, from key-on)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | 1 Hz, static: odometer, trip or service-interval candidate | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash)

#### 0x55A: Odometer (every 1000 ms, from key-on)

Test: Compare with the dash odometer.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0-D2 | Odometer (21430 = 2,143 km on the dash (390 Adventure). 1390 Evo: 00 17 16 = 591.0 km in sixteen logs and 00 17 83 = 601.9 km in a later one, so it counts up as the bike is ridden) | big-endian; x 0.1; km | Adv390: confirmed, 1390: capture |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5A0: Slow status (from key-on)

Seen on: KTM 690 Enduro R, 2020 (candidate), KTM 390 Adventure (Gen 2), 2026 (candidate), KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D1 | 690: D1 and D2 change once after the ECU wakes. 390 Adventure: static with the engine off. 1390 Evo: D3-D6 sit at 62 44 92 80 and D7 is a counter. | | |

Sources: [ktmcomp](https://github.com/ChickenPoptart/KTMComputer), [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5A2: Static with engine off (390 Adventure) (from key-on)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate), KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | capture again riding | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5B0: Static with engine off (390 Adventure) (from key-on)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate), KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | capture again riding | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5B1: Counter only (1390 Evo) (every 100 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D7 | only D7 changes (a counter) | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5B2: Static (1390 Evo) (every 100 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | always 08 00 00 00 00 00 00 00 | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5CF: Not decoded (1390 Evo) (every 100 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0-D5 change a little (six different frames in the whole set). Meaning not known. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5D0: Not decoded (1390 Evo) (every 100 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0 and D2 change (four different frames in the whole set). Meaning not known. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5D1: Suspension mode (semi-active) (every 20 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (confirmed)  
Test: On a bike with electronic suspension, change the suspension profile and watch D0.  
Note: Only bikes with electronic (semi-active) suspension: not a Gen 1 Duke.  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | Suspension profile (99 (Track), AA (Pro) and 00 (Automatic) were seen while switching profiles; the other values are from the repo README) | values: 0=Automatic, 34=Sport, 68=Street, 102=Comfort, 153=Track, 170=Pro, 187=Rain | 1390: confirmed |
| D1 | preload: 58 = auto standard in every log (the README also lists 60 auto low, 68 auto high, 50 = 100%, D8 = 95%, 48 = 90%, 08 = 10%, 90 = 5%, 00 = 0%) | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x5D2: Not decoded (1390 Evo) (every 100 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | D0-D5 change a little (six different frames in the whole set). Meaning not known. | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x6AB: Rolling security value? (1390 Evo) (every 1000 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | random-looking bytes with only two different frames in the whole set; probably a checksum or security value | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x6B9: Static with engine off (390 Adventure) (from key-on)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate), KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | capture again riding | | |

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash), [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x6A6 to 0x6A8: Multi-frame transport (dash?) (every 25 ms, from ?)

Seen on: KTM 390 Adventure (Gen 2), 2026 (candidate)  
Note: D0 = 0F or 30 looks like multi-frame framing: probably the dash or a diagnostic transport, not live sensor data.  

Sources: [adv390](https://github.com/jayasuryashaju/adv390-dash)

#### 0x6E2: Static (1390 Evo) (every 1000 ms, from ?)

Seen on: KTM 1390 Super Duke R Evo, 2024 (candidate)  

| Bytes | Meaning | How to read it | Bike: status |
| --- | --- | --- | --- |
| D0 | always 05 44 00 00 00 00 00 00 | | |

Sources: [ktm1390](https://github.com/sruon/ktm-1390-can)

#### 0x7DF: OBD-II request to all modules (from scan tool or this unit)

Note: Only appears while something is asking (a scan tool, or this unit's own requests).  

#### 0x7E0: Engine ECU: request (from scan tool or this unit)

Note: Confirmed on a 2015 390 Duke: the ECU listens on 7E0 and answers on 7E8.  

#### 0x7E8: Engine ECU: reply (from engine ECU)

Note: Only appears in answer to a request. Confirmed on a 2015 390 Duke.  

#### 0x7E1 to 0x7E7: Other module: request (from scan tool or this unit)

Note: Physical request to another module (ABS, dash), seen while a scanner or Find modules is running.  

#### 0x7E9 to 0x7EF: Other module: reply (from ABS, dash or similar)

Note: A module answering a request. Find modules lists who answered.
<!-- IDS-END -->
