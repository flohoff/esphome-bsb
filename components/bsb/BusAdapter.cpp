#include "esphome/core/log.h"
#include "BusAdapter.h"

namespace esphome {
namespace bsb {

static const char *TAG = "bsb";

void BusAdapter::setup() {
	uart_last_activity=millis();
}

#define FRAME_MAX_RETRY		3
#define FRAME_MAX_QUEUE_TIME	300
#define FRAME_RETRY_TIME	100

void BusAdapter::frame_tx(void ) {

	if (frame_tx_queue.empty())
		return;

	uint32_t now=millis();

	// waitfree = random(1,20) + 3 + 59;
	uint32_t delay=(rand()%20) + 3 + 59;

	if (uart_last_activity + delay > now)
		return;

	ESP_LOGD(TAG, "Frame tx: we have something in queue");

	BSBFrame *frame=frame_tx_queue.front();

	// We delay retrying by FRAME_RETRY_TIME
	if (frame->retry) {
		ESP_LOGD(TAG, "Frame tx: check for retry and retry time");
		if (frame->last_sent_millis + FRAME_RETRY_TIME > millis())
			return;
	}

	ESP_LOGD(TAG, "Frame tx: try %d - len %d from: %02x to: %02x type: %s Address: %08" PRIX32,
			frame->retry,
			frame->framelength(),
			frame->from(),
			frame->to(),
			frame->messagetypestring(),
			frame->address());
	ESP_LOGD(TAG, "Frame tx: %s", frame->hex().c_str());

	for (uint8_t byte : frame->buffer) {
		write_byte(byte^0xff);
	}

	now=millis();
	uart_last_activity=now;

	if (frame->retry==0) {
		frame->first_sent_millis=now;
	}
	frame->last_sent_millis=now;
	frame->retry++;

	// Did we exceed the retry time or the num retries
	if (frame->retry > FRAME_MAX_RETRY
			|| frame->first_sent_millis + FRAME_MAX_QUEUE_TIME < now) {

		ESP_LOGD(TAG, "Frame tx: dequeuing - exceeded trys or time");

		frame_tx_queue.pop_front();
		delete(frame);
	}
}

void BusAdapter::frame_queue(BSBFrame *frame) {
	frame_tx_queue.push_back(frame);
}

void BusAdapter::frame_process(void ) {

	ESP_LOGD(TAG, "Frame rx: len %d from: %02x to: %02x type: %s Address: %08" PRIX32,
			this->frame.framelength(),
			this->frame.from(),
			this->frame.to(),
			this->frame.messagetypestring(),
			this->frame.address());
	ESP_LOGD(TAG, "Frame: %s", this->frame.hex().c_str());

	if (!this->frame.crc_valid()) {
		ESP_LOGD(TAG, "Frame rx: crc invalid - dropping");

		this->frame.clear();
		return;
	}

	// Did we receive our own frame?
	if (!frame_tx_queue.empty()
		&& frame.from() == busaddress) {

		/* Check if this is the frame we just sent */
		BSBFrame *txframe=frame_tx_queue.front();

		if (txframe->address() == frame.address()) {
			ESP_LOGD(TAG, "Frame rx: Our own frame - dequeuing from tx queue - %08" PRIX32, frame.address());

			frame_tx_queue.pop_front();
			delete(txframe);

			this->frame.clear();
			return;
		}
	}

	/* Check if frame is targeted to us and dispatch to the registered callback */
	if (frame.to() == busaddress) {
		auto csensor=sensorcallbackmap.find(frame.address());
		if (csensor != sensorcallbackmap.end()) {
			csensor->second->receive(&frame);
		} else {
			auto cbinsensor=binarysensorcallbackmap.find(frame.address());
			if (cbinsensor != binarysensorcallbackmap.end()) {
				cbinsensor->second->receive(&frame);
			} else {
				ESP_LOGD(TAG, "Frame rx: no callback registered for address %08" PRIX32,
						frame.address());
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
