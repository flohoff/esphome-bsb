
Minimal proof of concept of a BSB (Boiler Side Bus) Bus Adapter component
for esphome.

Currently it only comes up - opens the UART and watches for incoming frames. Checking
the CRC and logging the frame as Debugging.

	[16:25:54][D][BSBBusAdapter:024]: Frame complete - len 11 from: 8a to: 00 type: Query Address: 0d3d0519
	[16:25:54][D][BSBBusAdapter:030]: Frame: DC.8A.00.0B.06.3D.0D.05.19.4F.8C (11)
	[16:25:54][D][BSBBusAdapter:035]: Message type: Kesseltemperatur

Building
========

My local building in docker.

    docker run --rm -v "${PWD}":/config -it ghcr.io/esphome/esphome run bsbba.yaml

esphome.yaml
============

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
