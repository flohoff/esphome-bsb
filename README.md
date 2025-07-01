
Minimal proof of concept of a BSB (Boiler Side Bus) Bus Adapter component
for esphome.

Currently it only comes up - opens the UART and watches for incoming frames. Checking
the CRC and logging the frame as Debugging.

    [09:05:43][D][BSBBusAdapter:018]: Frame complete - len 11 crc 0x0000
    [09:05:43][D][BSBBusAdapter:019]: Frame: DC.8A.00.0B.06.3D.0D.05.19.4F.8C (11)
    [09:05:43][D][BSBBusAdapter:018]: Frame complete - len 14 crc 0x0000
    [09:05:43][D][BSBBusAdapter:019]: Frame: DC.80.0A.0E.07.0D.3D.05.19.00.07.B9.10.EE (14)
    [09:05:53][D][BSBBusAdapter:018]: Frame complete - len 21 crc 0x0000
    [09:05:53][D][BSBBusAdapter:019]: Frame: DC.80.7F.15.02.2E.00.02.11.00.00.FF.FF.FF.FF.FF.FF.00.00.F8.F1 (21)
    [09:05:53][D][BSBBusAdapter:018]: Frame complete - len 21 crc 0x0000
    [09:05:53][D][BSBBusAdapter:019]: Frame: DC.80.7F.15.02.2F.00.02.11.00.00.FF.FF.FF.FF.FF.FF.00.00.83.90 (21)

== Building

My local building in docker.

    docker run --rm -v "${PWD}":/config -it ghcr.io/esphome/esphome run bsbba.yaml

== esphome.yaml

  uart:
    id: bsbuart
    rx_pin: GPIO16
    tx_pin: GPIO17
    parity: ODD
    stop_bits: 1
    data_bits: 8
    baud_rate: 4800

  BSBBusAdapter:
    uart_id: bsbuart

  external_components:
    - source:
        type: git
        url: https://github.com/flohoff/esphome-bsb
      components: [BSBBusAdapter]
