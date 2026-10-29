#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include <list>

namespace esphome {
namespace bsb {

enum ValueType {
	VT_unknown = 0,
	VT_float3div64tempC = 1,
	VT_percent = 2,
	VT_boolonoff = 3
};

#define BSB_STARTOFFRAME        0xdc
#define BSB_FRAME_MINSIZE	4
#define BSB_FRAME_PAYLOAD_OFF	9

#define CRC_SIZE	2
#define HDR_SIZE	5

typedef enum {
	MT_RINF = 0x1,
	MT_SINF = 0x2,
	MT_SET = 0x3,
	MT_ACK = 0x4,
	MT_NACK = 0x5,
	MT_QUERY = 0x6,
	MT_RESPONSE = 0x7,
	MT_ERROR = 0x8
} MType;

static const std::map<int,const char *> messagetypestringtable = {
	{ MT_RINF, "Request info" },
	{ MT_SINF, "Send info" },
	{ MT_SET, "Set" },
	{ MT_ACK, "Ack" },
	{ MT_NACK, "Nack" },
	{ MT_QUERY, "Query" },
	{ MT_RESPONSE, "Response" },
	{ MT_ERROR, "Error" }
};

class BSBFrame {
	public:
	BSBFrame(void) : retry(0), first_sent_millis(0), last_sent_millis(0) {
	}

	void clear(void ) {
		buffer.clear();
	}

	const char *messagetypestring(void ) const {
		if (messagetypestringtable.find(messagetype()) == messagetypestringtable.end())
				return "unknown";
		return messagetypestringtable.at(messagetype());
	}

	size_t buffersize(void ) {
		return buffer.size();
	}

	uint8_t to(void ) const {
		return this->buffer[2];
	}

	void to_set(uint8_t t) {
		this->buffer[2]=t;
	}

	void from_set(uint8_t f) {
		this->buffer[1]=f | 0x80;
	}

	uint8_t from(void ) const {
		return this->buffer[1] & 0x7f;
	}

	void append(uint8_t c) {
		buffer.push_back(c);
	}

	int framelength(void ) {
		return this->buffer[3];
	}

	void framelength_set(int payload) {
		this->buffer.assign(payload+CRC_SIZE+HDR_SIZE, 0x0);
		this->buffer[3]=payload+CRC_SIZE+HDR_SIZE;
	}


	bool complete(void ) {
		if (this->buffer.size() < BSB_FRAME_MINSIZE)
			return false;
		if (this->buffer.size() >= this->framelength())
			return true;
		return false;
	}

	bool crc_valid(void ) {
		uint16_t crc=crc16be(this->buffer.data(), this->buffer.size(),
				0, 0x1021, false, false);

		return (0x0 == crc);
	}

	void crc_calc(void ) {
		size_t	size=this->buffer.size();

		uint16_t crc=crc16be(this->buffer.data(), size-2,
				0, 0x1021, false, false);

		this->buffer[size-2]=crc>>8&0xff;
		this->buffer[size-1]=crc&0xff;
	}

	void finalize(void ) {
		this->buffer[0]=BSB_STARTOFFRAME;
		this->crc_calc();
	}


	std::string hex(void ) {
		return format_hex_pretty(this->buffer.data(), this->buffer.size());
	}

	void address_set(uint32_t addr) {

		if (needs_swap()) {
			addr=((addr&0xff000000)>>8)|((addr&0x00ff0000)<<8)|(addr&0x0000ffff);
		}

		this->buffer[5]=addr>>24&0xff;
		this->buffer[6]=addr>>16&0xff;
		this->buffer[7]=addr>>8&0xff;
		this->buffer[8]=addr&0xff;
	}

	uint32_t address(void ) {
		uint32_t        addr=0;

		for(int i=0;i<=3;i++) {
			addr<<=8;
			addr|=this->buffer[i+5];
		}

		// Query Swaps the first 2 bytes
		if (needs_swap()) {
			addr=((addr&0xff000000)>>8)|((addr&0x00ff0000)<<8)|(addr&0x0000ffff);
		}

		return addr;
	}

	bool needs_swap(void ) const {
		if (messagetype() == MT_QUERY)
			return true;
		if (messagetype() == MT_SET)
			return true;
		return false;
	}

	void messagetype_set(uint8_t msgtype) {
		this->buffer[4]=msgtype;
	}

	uint8_t messagetype(void ) const {
		return this->buffer[4];
	}

	uint8_t payload(int pos) const {
		return this->buffer[BSB_FRAME_PAYLOAD_OFF+pos];
	}

	void payload_set(int pos, uint8_t v) {
		this->buffer[BSB_FRAME_PAYLOAD_OFF+pos]=v;
	}

	public:
		uint8_t			retry;
		uint32_t		first_sent_millis;
		uint32_t		last_sent_millis;

		std::vector<uint8_t>	buffer;

};

class BusAdapter;

class Switch : public switch_::Switch, public Component {
	private:
	BusAdapter	*bus;
	uint32_t	address;

	public:
	void set_busadapter(BusAdapter *bus);
	void set_address(uint32_t address);
	protected:
	void write_state(bool state) override;
};

class Sensor : public PollingComponent, public sensor::Sensor {
	private:
	unsigned int	address;
	int		vtype;
	BusAdapter	*bus;

	public:
	Sensor(BusAdapter *bus, unsigned int address, int vtype);
	void update() override;
	void receive(BSBFrame *frame);
};

class BinarySensor : public PollingComponent, public binary_sensor::BinarySensor {
	private:
	unsigned int	address;
	int		vtype;
	BusAdapter	*bus;

	public:
	BinarySensor(BusAdapter *bus, unsigned int address, int vtype);
	void update() override;
	void receive(BSBFrame *frame);
};

class BusAdapter : public uart::UARTDevice, public Component {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    void frame_process(void );
    void frame_queue(BSBFrame *frame);
    void frame_tx(void );
    void bus_rx(void );
    void set_busaddress(uint8_t ba);
    void set_sensor_callback(Sensor *sensor, uint32_t address);
    void set_binarysensor_callback(BinarySensor *sensor, uint32_t address);
    uint8_t get_busaddress(void);
  private:
    BSBFrame frame;
    uint32_t uart_last_activity;
    uint8_t busaddress;
    std::list<BSBFrame*> frame_tx_queue;
    std::map<uint32_t,Sensor *> sensorcallbackmap;
    std::map<uint32_t,BinarySensor *> binarysensorcallbackmap;
};

}  // namespace bsb
}  // namespace esphome
