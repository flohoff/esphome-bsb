#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#include <stdint.h>
#include <map>
#include <string>

namespace esphome {
namespace bsb {

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

	uint8_t from(void ) const {
		return this->buffer[1];
	}

	void append(uint8_t c) {
		buffer.push_back(c);
	}

	int framelength(void ) {
		return this->buffer[3];
	}

#define BSB_FRAME_MINSIZE	4

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

	std::string hex(void ) {
		return format_hex_pretty(this->buffer.data(), this->buffer.size());
	}

	uint32_t address(void ) {
		uint32_t        addr=0;

		for(int i=0;i<=3;i++) {
			addr<<=8;
			addr|=this->buffer.at(i+5);
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

	uint8_t messagetype(void ) const {
		return this->buffer.at(4);
	}

	private:
		std::vector<uint8_t>	buffer;

};

class BSBRegister {
	public:
		uint32_t	address;
		uint8_t		type;
		uint8_t		access;
		const char	*name;

		void set_name(const char *n) {
			name=n;
		}
		void set_address(uint32_t a) {
			address=a;
		}
		void set_type(uint8_t t) {
			type=t;
		}
		void set_access(uint8_t a) {
			access=a;
		}

	enum RegisterType {
		float3div64tempC
	};

	enum RegisterAccess {
		ro,
		rw
	};
};


class BSBBusAdapter : public uart::UARTDevice, public Component {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    void frame_process(void );
    void add_register(BSBRegister *reg);
  private:
    BSBFrame frame;
    std::vector<BSBRegister*> registers;
};

}  // namespace bsb
}  // namespace esphome
