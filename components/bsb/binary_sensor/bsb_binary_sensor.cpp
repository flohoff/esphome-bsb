
#include "esphome/core/log.h"
#include "bsb_binary_sensor.h"

static const char *const TAG = "bsb_sensor";

namespace esphome {
namespace bsb {

	BinarySensor::BinarySensor(BusAdapter *bus, unsigned int address, int vtype)
		: address(address), vtype(vtype), bus(bus) {

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
		ESP_LOGD(TAG, "BinarySensor receive %s %08x %08" PRIX32,
				LOG_STR_ARG(this->get_component_log_str()),
				address, frame->address());

		/* ACK/NACK/ERROR carry no value - ACK payload positions
		   are covered by the CRC bytes */
		if (frame->messagetype() == MT_ACK
			|| frame->messagetype() == MT_NACK
			|| frame->messagetype() == MT_ERROR) {
			return;
		}

		if (vtype == VT_boolonoff) {
			/* 2-byte payload [enable, value] — e.g. 0x313D0571 Trinkwasserbetrieb */
			uint8_t onoff=frame->payload(1);
			ESP_LOGD(TAG, "VT_boolonoff payload(1)=0x%02x", onoff);

			publish_state(onoff);
		} else if (vtype == VT_bitonoff) {
			/* 3-byte status payload [on/off, 0, bitfield] — e.g. 0x31000212 Status Trinkwasserbereitung */
			uint8_t onoff=frame->payload(0);
			ESP_LOGD(TAG, "VT_bitonoff payload(0)=0x%02x", onoff);

			publish_state(onoff);
		}
	}
}
}
