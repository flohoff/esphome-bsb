
Minimal proof of concept of a BSB (Boiler Side Bus) Bus Adapter component
for esphome.

Currently it only comes up - opens the UART and watches for incoming frames. Checking
the CRC and logging the frame as Debugging.

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
