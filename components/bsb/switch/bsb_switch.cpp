
#include "bsb_switch.h"

static const char *const TAG = "bsb_switch";

namespace esphome {
namespace bsb {

void Switch::write_state(bool state) {
	ESP_LOGD(TAG, "write_state %d", state);

	BSBFrame *frame=new BSBFrame;

	frame->framelength_set(6);
	frame->to_set(0);
	frame->from_set(bus->get_busaddress());
	frame->messagetype_set(MT_SET);
	frame->address_set(address);

	/* Enable byte + value — exactly the room unit's 2-byte SET payload */
	frame->payload_set(0, 0x01);
	frame->payload_set(1, state ? 1 : 0);

	frame->finalize();

	bus->frame_queue(frame);
	this->publish_state(state);
}

void Switch::setup() {
	bus->set_switch_callback(this, address);
}

void Switch::update() {
	BSBFrame *frame=new BSBFrame;

	frame->framelength_set(4);
	frame->to_set(0);
	frame->from_set(bus->get_busaddress());
	frame->messagetype_set(MT_QUERY);
	frame->address_set(address);
	frame->finalize();

	bus->frame_queue(frame);
}

void Switch::receive(BSBFrame *frame) {
	/* Only responses and info frames carry a value */
	if (frame->messagetype() != MT_RESPONSE
		&& frame->messagetype() != MT_SINF) {
		return;
	}

	/* 2-byte payload [enable, value] */
	bool state=frame->payload(1) != 0;
	ESP_LOGD(TAG, "receive state %d", state);
	this->publish_state(state);
}

void Switch::set_address(uint32_t address) {
	this->address=address;
}

void Switch::set_busadapter(BusAdapter *bus) {
	this->bus=bus;
}

}
}
