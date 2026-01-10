
#include "esphome/core/log.h"
#include "bsb_binary_sensor.h"

static const char *const TAG = "bsb_sensor";

namespace esphome {
namespace bsb {

	BinarySensor::BinarySensor(BusAdapter *bus, unsigned int address, int vtype)
		: bus(bus), address(address), vtype(vtype) {

		bus->set_binarysensor_callback(this, address);

		if (vtype == VT_boolonoff) {
		}	
	};

	void BinarySensor::update(void) {
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

	void BinarySensor::receive(BSBFrame *frame) {
		ESP_LOGD(TAG, "BinarySensor receive %s %08x %08x",
				LOG_STR_ARG(this->get_component_log_str()),
				address, frame->address());

		if (frame->messagetype() == MT_ERROR) {
			return;
		}

		if (vtype == VT_boolonoff) {
			ESP_LOGD(TAG, "Sensor VT_boolonoff %s %08x ",
				LOG_STR_ARG(this->get_component_log_str()),
				address);

			uint8_t onoff=frame->payload(0)|frame->payload(1);
			publish_state(onoff);
		}
	}
}
}
