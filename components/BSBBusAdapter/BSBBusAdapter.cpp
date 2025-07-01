#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "BSBBusAdapter.h"

namespace esphome {
namespace bsb {

static const char *TAG = "BSBBusAdapter";

void BSBBusAdapter::setup() {

}
#define BSB_STARTOFFRAME        0xdc

void BSBBusAdapter::frame_complete(void ) {
    uint16_t crc=crc16be(this->_buffer.data(), this->_buffer.size(), 0, 0x1021, false, false);

    ESP_LOGD(TAG, "Frame complete - len %d crc 0x%04x", this->_buffer.size(), crc);
    ESP_LOGD(TAG, "Frame: %s", format_hex_pretty(this->_buffer.data(), this->_buffer.size()).c_str());
}

void BSBBusAdapter::loop() {
	if (!available())
		return;

	uint8_t c;
	while (available()) {
		read_byte(&c);
		c^=0xff;

		if (c == BSB_STARTOFFRAME) {
			this->_buffer.clear();
		}

		this->_buffer.push_back(c);

		if (this->_buffer.size() < 4)
			continue;

		int framelength=this->_buffer[3];

		if (this->_buffer.size() >= framelength)
			frame_complete();
	}
}

void BSBBusAdapter::dump_config(){
    ESP_LOGCONFIG(TAG, "BSBBusAdapter");
}

}  // namespace bsb
}  // namespace esphome
