# esphome-bsb

A BSB (Boiler System Bus) bus adapter component for
[esphome](https://esphome.io) - Brötje / Elco heater family (WBS, Thision
etc.).

The component speaks the BSB protocol over a serial line (4800 baud, 8 data
bits, odd parity, 1 stop bit, payload bits inverted) and lets esphome both
read parameters from and write parameters to the heating system.

## What it can do

The component offers five esphome platforms. Every entity polls its
parameter with a QUERY frame (`update_interval`, default 60s); the switch,
select and number platforms additionally send SET frames when controlled:

- **`binary_sensor`** - read-only status bits
- **`sensor`** - polled read-only values (temperature, percent, ...)
- **`switch`** - boolean on/off setpoints (e.g. domestic hot water)
- **`select`** - enum values (e.g. operating mode), rendered as a dropdown
- **`number`** - numeric setpoints (e.g. set temperatures), rendered as a slider

### Value types (`vtype`)

The `vtype` selects how the payload of the answer frames is decoded and how
SET payloads are built. It must match the datatype the heater uses for the
parameter in question - look it up in the
[BSB-LAN parameter database](https://github.com/fredlcore/BSB-LAN/blob/master/BSB_LAN/BSB_LAN_custom_defs.h.default)
(`BSB_LAN_custom_defs.h.default`, the comment column names the parameter).

| vtype               | BSB-LAN type | payload (ANS)                        | unit |
|---------------------|--------------|--------------------------------------|------|
| `boolonoff`         | VT_ENUM (on/off) | 2 byte `[enable, value]`         | on/off |
| `bitonoff`          | VT_BIT       | 3 byte `[on/off, 0, bitfield]`       | on/off |
| `percent`           | VT_PERCENT   | 2 byte `[enable, value]`             | %    |
| `float3-div64-tempC`| VT_TEMP      | 3 byte `[enable, hi, lo]`, value/64, signed | °C |
| `enum`              | VT_ENUM      | 2 byte `[enable, value]`             | per optionsmap |

The switch/select/number platforms know where the value byte lives for each
vtype - there are no per-entity payload offsets to configure. For `enum`,
the mapping from dropdown entry to BSB value is explicit in the yaml
(`optionsmap`, see the example config) so different heaters can use
different value assignments.

### Addresses and the wire format

Parameters are addressed in canonical form `0x<family><param>` - e.g.
`0x313D0571` (family `0x31` = Brötje) for "Trinkwasserbetrieb Ein/Aus".
On the wire, QUERY and SET frames swap the first two address bytes
(family comes second); ACK and ANSW frames echo the canonical form. The
component handles the swap - use the canonical form from the BSB-LAN
database in the yaml.

Frames are `0xDC`-delimited, CRC16 (poly 0x1021) protected and queued with
retry/backoff. Details and the reverse engineering notes are in
[STATUS.md](STATUS.md).

## Configuration

A complete, working example config for a Brötje WBS 14E - including the
bus adapter, all platforms, the external_components block and the device
settings - lives in [heizung.yaml](heizung.yaml) in this repository.

## Building

With docker:

    docker run --rm -v "${PWD}":/config -it ghcr.io/esphome/esphome run bsbba.yaml

or a local esphome venv (2026.10.x):

    esphome compile bsbba.yaml
    esphome upload bsbba.yaml            # OTA
    esphome upload bsbba.yaml --device /dev/ttyUSB0    # USB fallback

In the ESPHome Device Builder add-on, point `external_components` at this
repository:

    external_components:
      - source:
          type: git
          url: https://github.com/flohoff/esphome-bsb
        components: [bsb]

Note: without a `refresh:` interval, git sources are only re-fetched once a
day - add `refresh: 1s` while developing, and keep the add-on on a recent
esphome version (2026.10 or newer; the select/number codegen APIs are
newer than 2025 releases).

## Copyright

Copyright (c) 2026 Florian Lohoff <f@zz.de>

As this was 70% implemented by the LLM GLM 5.3-flash i consider this in the Public Domain
or if your jurisdiction does not know about it i am happy with Creative Commons CC0.

