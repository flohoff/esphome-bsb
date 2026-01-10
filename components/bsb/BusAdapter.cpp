#include "esphome/core/log.h"
#include "BusAdapter.h"

namespace esphome {
namespace bsb {

static const char *TAG = "bsb";

void BusAdapter::setup() {
	uart_last_activity=millis();
}

void BusAdapter::frame_tx(void ) {

	if (frame_tx_queue.empty())
		return;

	BSBFrame *frame=frame_tx_queue.front();
	frame_tx_queue.pop_front();

	ESP_LOGD(TAG, "Frame complete - len %d from: %02x to: %02x type: %s Address: %08x",
			this->frame.buffersize(),
			this->frame.from(),
			this->frame.to(),
			this->frame.messagetypestring(),
			this->frame.address());

	ESP_LOGD(TAG, "Frame: %s", frame->hex().c_str());

	for (uint8_t byte : frame->buffer) {
		write_byte(byte^0xff);
	}

	uart_last_activity=millis();

	delete(frame);
}

void BusAdapter::frame_queue(BSBFrame *frame) {
	frame_tx_queue.push_back(frame);
}

void BusAdapter::frame_process(void ) {
	if (!this->frame.crc_valid()) {

		ESP_LOGD(TAG, "Frame crc invalid - len %d", this->frame.buffersize());
		ESP_LOGD(TAG, "Frame: %s", this->frame.hex().c_str());

		this->frame.clear();
		return;
	}

	ESP_LOGD(TAG, "Frame complete - len %d from: %02x to: %02x type: %s Address: %08x",
			this->frame.buffersize(),
			this->frame.from(),
			this->frame.to(),
			this->frame.messagetypestring(),
			this->frame.address());
	ESP_LOGD(TAG, "Frame: %s", this->frame.hex().c_str());

	/* Check if frame is targeted to us and is the expected last requests address */
	if (frame.to() == busaddress) {
		Sensor *cbsensor=sensorcallbackmap.find(frame.address())->second;
		if (cbsensor) {
			cbsensor->receive(&frame);
		} else {
			BinarySensor *cbbinsensor=binarysensorcallbackmap.find(frame.address())->second;
			if (cbbinsensor) {
				cbbinsensor->receive(&frame);
			}
		}
	}
	this->frame.clear();
}

void BusAdapter::bus_rx(void ) {
	uint8_t c;
	while (available()) {
		read_byte(&c);

		/* Payload bits are inverted */
		c^=0xff;

		if (c == BSB_STARTOFFRAME) {
			this->frame.clear();
		}

		this->frame.append(c);
		if (this->frame.complete())
			frame_process();
	}
}

void BusAdapter::loop() {
	if (available()) {
		this->bus_rx();
		uart_last_activity=millis();
		return;
	}

#define UART_SILENT_MILLIS	10

	uint32_t now=millis();
	if (uart_last_activity + UART_SILENT_MILLIS > now)
		return;

	this->frame_tx();
}

void BusAdapter::dump_config(){
    ESP_LOGCONFIG(TAG, "BusAdapter");
}

void BusAdapter::set_sensor_callback(Sensor *sensor, uint32_t address) {
	sensorcallbackmap.insert(std::pair<uint32_t,Sensor *>(address, sensor));
}

void BusAdapter::set_binarysensor_callback(BinarySensor *sensor, uint32_t address) {
	binarysensorcallbackmap.insert(std::pair<uint32_t,BinarySensor *>(address, sensor));
}

void BusAdapter::set_busaddress(uint8_t ba) {
	busaddress=ba;
}

uint8_t BusAdapter::get_busaddress() {
	return busaddress;
}


}  // namespace bsb
}  // namespace esphome
