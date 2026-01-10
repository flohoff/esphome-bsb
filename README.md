
Minimal proof of concept of a BSB (Boiler Side Bus) Bus Adapter component
for esphome.

Building
========

My local building in docker.

    docker run --rm -v "${PWD}":/config -it ghcr.io/esphome/esphome run bsbba.yaml

esphome.yaml
============

Here an example for an Brötje WBS 14E collecting some infos:

    uart:
      id: bsbuart
      rx_pin: GPIO16
      tx_pin: GPIO17
      parity: ODD
      stop_bits: 1
      data_bits: 8
      baud_rate: 4800
  
    bsb:
      id: bsb0
      uart_id: bsbuart
      busaddress: 0xe

    binary_sensor:
      - platform: bsb
        bsb_id: bsb0
        name: Brennerstufe 1
        address: 0x053D09A0
        vtype: boolonoff

    sensor:
      - platform: bsb
        bsb_id: bsb0
        name: Kesseltemperatur
        address: 0x0D3D0519
        vtype: float3-div64-tempC
      - platform: bsb
        bsb_id: bsb0
        name: Aussentemperatur
        address: 0x053D0521
        vtype: float3-div64-tempC
     
    external_components:
      - source:
          type: git
          url: https://github.com/flohoff/esphome-bsb
        components: [BSBBusAdapter]
