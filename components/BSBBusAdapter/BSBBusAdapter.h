#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace bsb {
class BSBBusAdapter : public uart::UARTDevice, public Component {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    void frame_complete(void );
  private:
    std::vector<uint8_t> _buffer;
};
}  // namespace bsb
}  // namespace esphome
