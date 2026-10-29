# STATUS — bsb-esphome

Stand: 2026-10-29, ~17:00 Uhr

## Kurzfassung

Die BSB-Komponente läuft wieder unter ESPHome 2026.10.1. Drei Bugs sind gefixt
(Sensor-Einheiten, UB-Crash im BusAdapter, Switch-Zieladresse/-Payload), der
„Status Trinkwasserbereitung"-Switch schaltet das TWW live am Kessel (ACK in
beide Richtungen verifiziert). Neu: eine `select`-Plattform für die
Heizkreis-Betriebsart (`0x2D3D0574`) — **geflasht und live verifiziert**:
Betriebsart-Wechsel per Select wurden am Kessel-Frontpanel bestätigt, der
Select-Poll liest den aktuellen Modus korrekt aus (Stand: „Automatik").
Der ESP32 war zwischenzeitlich offline (No route to host) und wurde per Reset
wiederbelebt; als Fallback ist er jetzt zusätzlich direkt per USB (`ttyUSB0`)
mit diesem Rechner verbunden.

## Umgebung

- Repo: `/home/flo/bsb-esphome`, Platform: linux, Gerät: ESP32 `nodemcu-32s`
- Device: `bsbba.fritz.box` → 192.168.178.163, MAC 7C:9E:BD:65:F5:E8, Hostname `bsbba`, SSID `b1i.zz.de`
- Framework: esp-idf (IDF 5.5.6), ESPHome 2026.10.1 (venv: `venv/`, Python 3.13.5)
- Seriell: ESP32 direkt mit diesem Rechner verbunden über `ttyUSB0`
  (Upload/Reset per USB als Fallback: `venv/bin/esphome upload bsbba.yaml --device /dev/ttyUSB0`)
- Bus-Adressen: Adapter = `0x0e`, Kessel = `0x00`, Raumgerät/Bedienteil = `0x0a`, Broadcast = `0x7f`
- Kessel: Brötje WBS 14E (Familie `0x31`), Referenz-Checkout des Users: `~/BSB-LAN`
  (Parameter-DB: `~/BSB-LAN/BSB_LAN/BSB_LAN_custom_defs.h.default`, Lokalisierung: `BSB_LAN/localization/LANG_DE.h`)
- Git: Benutzer hat die früheren Fixes committed; die Select-Arbeit ist aktuell
  uncommitted im Arbeitsbaum.

## BSB-Bus: Erkenntnisse

### Frame-Format (aus Live-Captures verifiziert)

```
Byte 0      Startbyte 0xDC
Byte 1      0x80 | Absenderadresse
Byte 2      Zieladresse
Byte 3      Gesamtlänge (inkl. CRC)
Byte 4      Typ: 02=INF 03=SET 04=ACK 05=NACK 06=QUERY 07=RESPONSE 08=ERROR
Byte 5-8    Kommando (4 Byte)
Byte 9..n-3 Payload (Länge = Byte 3 - 11)
letzte 2    CRC (crc16be, poly 0x1021)
```

- QUERY/SET tauschen auf der Leitung die ersten beiden Kommando-Bytes
  (`family.param` wird als `param_hi.family.param_lo.param_lolo` gesendet);
  Kessel-ACK/ANS antworten in kanonischer Form (Familie zuerst).
  `BSBFrame::address_set/address()` bildet das über `needs_swap()` ab.
- Eigene Frames (Echo) werden über die TX-Queue erkannt und rausgereicht,
  nicht an Callbacks verteilt.

### Parameter (live verifiziert)

| Adresse       | Bedeutung (bsb-lan-DB)                    | Payload (ANS)        | schreibbar |
|---------------|-------------------------------------------|----------------------|------------|
| `0x0D3D0519`  | Kesseltemperatur (VT_float3div64tempC)    | Wert/64 → °C         | (lesen)    |
| `0x2D3D0571`  | Brennermodulation (percent)               | %                    | (lesen)    |
| `0x313D0571`  | VT_ENUM „Trinkwasserbetrieb Ein/Aus" (ProgNr 1600) | 2 Byte `[enable, value]` | **ja** |
| `0x31000212`  | VT_BIT „Status Trinkwasserbereitung" (ProgNr 1602) | 3 Byte `[on/off, 0, bitfield]` | nein (NACK) |
| `0x2D3D0574`  | VT_ENUM „Heizkreis 1 - Betriebsart" (ProgNr 700)   | 2 Byte `[enable, value]` | **ja** |
| `0x313D06B9`  | VT_TEMP „Trinkwasser - Nennsollwert" (ProgNr 1610) | 3 Byte `[enable, hi, lo]`, Wert×64 | **ja** |
| `0x2D3D058E`  | VT_TEMP „Heizkreis 1 - Komfortsollwert" (ProgNr 710) | 3 Byte `[enable, hi, lo]`, Wert×64 | **ja** |
| `0x2D3D0572`  | „Heizkreis 1 - Präsenztaste" (FL_WONLY)   |                      | (noch ungenutzt) |

- TWW: SET `[01, 01]` = Ein, `[01, 00]` = Aus (Raumgerät-Referenzframes:
  `DC.8A.00.0D.03.3D.31.05.71.01.0X.<CRC>` — 13 Byte, alle ACKed).
- Temperatur-SET (Raumgerät-Mitschnitt, Komfortsollwert 21→20 °C):
  14-Byte-Frame, Payload `[01, hi, lo]` mit Wert×64
  (`0x0540` = 21.0 °C, `0x0500` = 20.0 °C) — identisch zur ANS-Kodierung.
- Betriebsart: Modus-Werte aus 8 Knopfdrücken des Raumgeräts (2× durchlaufen):
  `3 → 2 → 0 → 1`. Lokalisierung (LANG_DE.h, ENUM700): **0 = Schutzbetrieb,
  1 = Automatik, 3 = Komfort**; Wert 2 hat in der bsb-lan-Referenz keinen
  Namen — vermutlich „Absenken/Eco" (im YAML vorbefüllt, dort umbenennbar).

### NACK-Fehlercodes (beobachtet)

- Fehlercode `05` im NACK-Payload: SET auf das nur-lesbare Status-Register
  `0x31000212` (korrektes 2-Byte-Payload).
- Fehlercode `08`: SET mit falschem Payload (3 Byte statt 2).
- D. h. die Codes differenzieren bereits zwischen „Parameter nicht
  schreibbar" und „ungültiger Wert/Länge" — genaue Tabelle noch offen.

## Diese Session gefixt

1. **Sensor-Einheiten** (ESPHome 2026.10 hat `Sensor::set_unit_of_measurement()`
   im C++ entfernt): Einheiten werden jetzt in `to_code()` über
   `config[CONF_UNIT_OF_MEASUREMENT]` gesetzt, Mapping in
   `vtype_to_unit()` anhand `vtype.enum_value` (direkter Vergleich des
   `EnumValue`-Wrappers ist immer truthy — Falle).
   Verifiziert: `Kesseltemperatur → uom:°C`, `Brennermodulation → uom:%`.
2. **UB-Crash** in `BusAdapter::frame_process`:
   `sensorcallbackmap.find(addr)->second` dereferenzierte `end()`, wenn für
   eine Adresse kein Callback registriert war (jeder fremde Frame → Crash).
   Jetzt `find()` + `!= end()`-Guard.
3. **frame_tx-Log**: loggte `this->frame.*` statt `frame->*` (falsche Werte
   im Log).
4. **Switch „Status Trinkwasserbereitung"**: zielte auf das schreibgeschützte
   Status-Register `0x31000212` (→ NACK 05) und schickte durch ein
   zurückgebliebenes `payload_set(2, 0x00)` + `framelength_set(7)` einen
   14-Byte-Frame mit 3-Byte-Payload (→ NACK 08). Jetzt: Adresse
   `0x313D0571`, `framelength_set(6)`, Payload `[0x01, state]` — byteidentisch
   zum Raumgerät. Live verifiziert: Ein- und Ausschalten ACKed.
5. **Neuer Typ `VT_bitonoff`** (YAML: `vtype: bitonoff`): 3-Byte-Statusform,
   Wert in Byte 0. Davor las der Status-Sensor bei `boolonoff` immer Byte 1
   (= `0x00`) → dauerhaft OFF. Jetzt korrekt ON/OFF.
6. **ACK/NACK/ERROR-Guard** in `BinarySensor::receive`: ACK-Frames haben
   keinen Payload — die Payload-Position liegt dort auf den CRC-Bytes, der
   Sensor published sonst nach jedem SET kurz CRC-Müll (bis zur nächsten
   Poll-Korrektur).
7. **Neue Plattform `number`** (`components/bsb/number/`): setzbare Zahlenwerte
   (z. B. Solltemperaturen). `Number : PollingComponent, number::Number`,
   `control(float)` sendet SET `[0x01, hi, lo]` mit `lroundf(value*64)`,
   `receive()` dekodiert `payload(1)<<8|payload(2)`, /64, publish (nur
   RESPONSE/SINF; ACK/NACK/ERROR werden ignoriert). Dispatch über neue
   `numbercallbackmap`. YAML: `min_value`/`max_value`/`step` (Required/Optional,
   Step-Default 0.5), `unit_of_measurement="°C"` fest im Schema. Live
   verifiziert: TWW Nennsollwert 50→55→50 (SET `[01, 0D, C0]` = 55×64, ACKed),
   Komfortsollwert liest 20.0 °C.
   Der bisherige read-only-Sensor „TWW Nennsollwert" wurde durch das
   number-Entity gleichen Namens ersetzt; neu: „Komfortsollwert Heizkreis 1".
   Web-API-Quirks: Select-Set nutzt `?option=`, Number-Set `?value=`, und
   POST braucht `-d ""` (sonst HTTP 411).
8. **Neue Plattform `select`** (`components/bsb/select/`): Dropdown für
   Enum-Parameter. `Select : PollingComponent, select::Select`,
   `control(size_t)` sendet SET `[enable, mapping[index]]`, `receive()` published
   bei RESPONSE/SINF `payload(1)` rückgemappt über die `optionsmap` (nur
   `VT_enum`), Dispatch über neue `selectcallbackmap` im BusAdapter. YAML:
   `optionsmap` als Dict `{Betriebsart: BSB-Wert}` (Muster:
   `modbus_controller`-Select); Labels + Mapping-Liste kommen aus dem Dict,
   ESPHome legt sie automatisch ins Progmem. Live verifiziert: SET
   `[01, 02]` (Absenken) und `[01, 01]` (Automatik) beide ACKed, Rücklesen
   springt sauber mit.

## Behoben: Gerät war offline

- Zwischen ~16:43 und ~16:47 offline (`No route to host` auf Port 3232, kein
  Ping); Ursache ungeklärt (Strom/WLAN/Crash). Die fehlgeschlagenen Uploads
  kamen nicht über den Connect — es wurde nichts geschrieben.
- Vom Benutzer per Reset wiederbelebt; WLAN wieder online. Der letzte
  Stand des Select-Codes wurde vom Benutzer geflasht und am Kessel-Frontpanel
  verifiziert (Betriebsart-Wechsel sichtbar).

## Offene Punkte / Next Steps

1. **Select/switch/number-Arbeit committen** — erledigt: Commits bis `30d6a36`
   inkl. `cc1f5c4` sind gepusht (Origin + GitHub-Spiegel aktuell ✓); der
   aktuelle Arbeitsbaum (number-Plattform, `optionsmap`, number-Entities,
   README) wartet auf den nächsten Commit + Push.
2. Betriebsart-`optionsmap` pro Heizgerät: Werte sind jetzt explizit im YAML
   (`{Betriebsart: BSB-Wert}`); für andere Heizgeräte ggf. neu ermitteln
   (Bus-Mitschnitt: `type: Set`-Frames beim Umschalten). — erledigt:
   `optionsmap`-Muster (Vorbild `modbus_controller`-Select) implementiert,
   im YAML dokumentiert.
3. NACK-Fehlercodetabelle vervollständigen (bsb-lan-Quellen; beobachtet:
   05 = Parameter nicht schreibbar, 08 = ungültiger Wert/Länge).
4. `0x2D3D0572` (Präsenztaste) und weitere Parameter nach Bedarf erschließen.

## Referenzen

- `~/BSB-LAN` — bsb-lan-Checkout des Users („moved back in git history"),
  enthält die Parameter-DB mit Typen/ProgNr/ENUM-Texten.
- `/tmp/opencode/` — Session-Artefakte (Captures/Analysen, flüchtig!):
  `bus_watch2.txt` (Knopfdruck-TWW, 8 SETs), `mode_watch.txt`
  (Betriebsart-Zyklus), `set_*.txt`, `toggle_test*.txt`, `bitonoff_test.txt`,
  `bsb_lan_main.cpp` (bsb-lan-Referenzquellen), `bsb-wiki*` (geclonte Wikis).

## Kommandos

```bash
./venv/bin/esphome config  bsbba.yaml    # Validierung
./venv/bin/esphome compile bsbba.yaml    # Build
./venv/bin/esphome upload  bsbba.yaml    # OTA-Flash
./venv/bin/esphome logs    bsbba.yaml    # Live-Log (DEBUG: Frame-Hexdumps)
curl -s -X POST -d "" "http://bsbba.fritz.box/switch/Status%20Trinkwasserbereitung/toggle"
curl -s "http://bsbba.fritz.box/binary_sensor/Status%20Trinkwasserbereitung%20Sensor"
curl -s -X POST "http://bsbba.fritz.box/select/Betriebsart%20Heizkreis%201/set?value=Automatik"
```
