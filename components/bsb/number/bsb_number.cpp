
#include "esphome/core/log.h"
#include "bsb_number.h"

static const char *const TAG = "bsb_number";

namespace esphome {
namespace bsb {

	Number::Number(BusAdapter *bus, unsigned int address, int vtype)
		: address(address), vtype(vtype), bus(bus) {

		bus->set_number_callback(this, address);
	};

	void Number::update(void) {
		ESP_LOGD(TAG, "Update %s %08x",
				LOG_STR_ARG(this->get_component_log_str()),
				address);

		BSBFrame *frame=new BSBFrame;

		frame->framelength_set(4);
		frame->to_set(0);
		frame->from_set(bus->get_busaddress());
		frame->messagetype_set(MT_QUERY);
		frame->address_set(address);
		frame->finalize();

		bus->frame_queue(frame);
	}

	void Number::receive(BSBFrame *frame) {
		ESP_LOGD(TAG, "Number receive %s %08x %08" PRIX32,
				LOG_STR_ARG(this->get_component_log_str()),
				address, frame->address());

		/* Only responses and info frames carry a value */
		if (frame->messagetype() != MT_RESPONSE
			&& frame->messagetype() != MT_SINF) {
			return;
		}

		if (vtype == VT_float3div64tempC) {
			/* 3-byte payload [enable, hi, lo] - value/64 °C
			   e.g. 0x313D06B9 TWW Nennsollwert, 0x2D3D058E Komfortsollwert */
			int16_t rawnum=(frame->payload(1) << 8) | frame->payload(2);

			float temp=rawnum/64.0f;

			ESP_LOGD(TAG, "Number VT_float3div64tempC payload(1,2)=0x%04x -> %.2f", rawnum, temp);

			this->publish_state(temp);
		}
	}

	void Number::control(float value) {
		ESP_LOGD(TAG, "control %s %.2f",
				LOG_STR_ARG(this->get_component_log_str()),
				value);

		BSBFrame *frame=new BSBFrame;

		frame->framelength_set(7);
		frame->to_set(0);
		frame->from_set(bus->get_busaddress());
		frame->messagetype_set(MT_SET);
		frame->address_set(address);

		/* Enable byte + 16-bit value/64 - the 3-byte form the room unit uses */
		uint16_t rawnum=(uint16_t) lroundf(value*64.0f);

		frame->payload_set(0, 0x01);
		frame->payload_set(1, rawnum >> 8);
		frame->payload_set(2, rawnum & 0xff);

		frame->finalize();

		bus->frame_queue(frame);
		this->publish_state(value);
	}
}
}
