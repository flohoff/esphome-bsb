
#include "esphome/core/log.h"
#include "bsb_select.h"

static const char *const TAG = "bsb_select";

namespace esphome {
namespace bsb {

	Select::Select(BusAdapter *bus, unsigned int address, int vtype, std::vector<uint8_t> mapping)
		: address(address), vtype(vtype), mapping(mapping), bus(bus) {

		bus->set_select_callback(this, address);
	};

	void Select::update(void) {
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

	void Select::receive(BSBFrame *frame) {
		ESP_LOGD(TAG, "Select receive %s %08x %08" PRIX32,
				LOG_STR_ARG(this->get_component_log_str()),
				address, frame->address());

		/* Only responses and info frames carry a value */
		if (frame->messagetype() != MT_RESPONSE
			&& frame->messagetype() != MT_SINF) {
			return;
		}

		if (vtype == VT_enum) {
			/* 2-byte payload [enable, value] - e.g. 0x2D3D0574 Betriebsart */
			uint8_t value=frame->payload(1);

			ESP_LOGD(TAG, "Select VT_enum payload(1)=0x%02x", value);

			/* Map the BSB value back to the dropdown index via the optionsmap */
			size_t index;
			if (this->mapping.empty()) {
				index=value;
			} else {
				auto it=std::find(this->mapping.begin(), this->mapping.end(), value);
				if (it == this->mapping.end()) {
					ESP_LOGD(TAG, "Select value 0x%02x not in optionsmap - ignoring", value);
					return;
				}
				index=it - this->mapping.begin();
			}

			if (index < this->size()) {
				this->publish_state(index);
			}
		}
	}

	void Select::control(size_t index) {
		ESP_LOGD(TAG, "control %s index %zu",
				LOG_STR_ARG(this->get_component_log_str()),
				index);

		/* Map the dropdown index to the BSB value via the optionsmap */
		uint8_t value=(index < this->mapping.size()) ? this->mapping[index] : index;

		BSBFrame *frame=new BSBFrame;

		frame->framelength_set(6);
		frame->to_set(0);
		frame->from_set(bus->get_busaddress());
		frame->messagetype_set(MT_SET);
		frame->address_set(address);

		/* Enable byte + value - the 2-byte form the room unit uses */
		frame->payload_set(0, 0x01);
		frame->payload_set(1, value);

		frame->finalize();

		bus->frame_queue(frame);
		this->publish_state(index);
	}
}
}
