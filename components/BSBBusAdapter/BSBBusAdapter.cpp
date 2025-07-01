#include "esphome/core/log.h"
#include "BSBBusAdapter.h"

namespace esphome {
namespace bsb {

static const char *TAG = "BSBBusAdapter";

void BSBBusAdapter::setup() {

}
#define BSB_STARTOFFRAME        0xdc

void BSBBusAdapter::frame_process(void ) {
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

	uint32_t address=frame.address();
	for(auto r : this->registers)
		if (r->address == address) {
			ESP_LOGD(TAG, "Message type: %s", r->name);
		}

}

void BSBBusAdapter::loop() {
	if (!available())
		return;

	uint8_t c;
	while (available()) {
		read_byte(&c);
		c^=0xff;

		if (c == BSB_STARTOFFRAME) {
			this->frame.clear();
		}

		this->frame.append(c);
		if (this->frame.complete())
			frame_process();
	}
}

void BSBBusAdapter::dump_config(){
    ESP_LOGCONFIG(TAG, "BSBBusAdapter");
}

void BSBBusAdapter::add_register(BSBRegister *reg) {
	registers.push_back(reg);
}

}  // namespace bsb
}  // namespace esphome
