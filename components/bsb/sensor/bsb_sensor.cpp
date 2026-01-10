
#include "esphome/core/log.h"
#include "bsb_sensor.h"

static const char *const TAG = "bsb_sensor";

namespace esphome {
namespace bsb {

	Sensor::Sensor(BusAdapter *bus, unsigned int address, int vtype)
		: bus(bus), address(address), vtype(vtype) {

		bus->set_sensor_callback(this, address);

		if (vtype == VT_float3div64tempC) {
			this->set_device_class("temperature");
			this->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
			this->set_accuracy_decimals(2);
			this->set_unit_of_measurement("\302\260C");
		} else if (vtype == VT_percent) {
			this->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
			this->set_accuracy_decimals(0);
			this->set_unit_of_measurement("%");
		}
	};

	void Sensor::update(void) {
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

	uint16_t sign_extend(uint16_t value, uint16_t bits) {
	    assert(bits > 0 && bits < 16);
	    uint16_t sign = (1 << (bits - 1)) & value;
	    uint16_t mask = ((~0U) >> (bits - 1)) << (bits - 1);
	    if (sign != 0)
		value |= mask;
	    else
		value &= ~mask;
	    return value;
	}

	void Sensor::receive(BSBFrame *frame) {
		ESP_LOGD(TAG, "Sensor receive %s %08x %08x",
				LOG_STR_ARG(this->get_component_log_str()),
				address, frame->address());

		if (frame->messagetype() == MT_ERROR) {
			//publish_state(NAN);
			return;
		}

		if (vtype == VT_float3div64tempC) {

			/* Although 3 byte payload only the last 
			 * 2 bytes are actually in use - seen with
			 * negative numbers e.g. 0x00 0xff 0xfd 
			 *
			 * So only process the last 2 bytes in
			 * the 3 byte payload
			 */
			int16_t rawnum=frame->payload(1) << 8 |
					frame->payload(2);

			/* Sign extend to integer */
			int	num=rawnum;
			/* Make it a flow div 64 */
			double	temp=((double) num)/64;

			ESP_LOGD(TAG, "Sensor VT_float3div64tempC receive %s %08x %08x %.2f°C",
				LOG_STR_ARG(this->get_component_log_str()),
				address, frame->address(), num, temp);

			publish_state(temp);
		} else if (vtype == VT_percent) {
			ESP_LOGD(TAG, "Sensor VT_percent %s %08x ",
				LOG_STR_ARG(this->get_component_log_str()),
				address);

			float num=frame->payload(1);
			publish_state(num);
		}
	}
}
}
