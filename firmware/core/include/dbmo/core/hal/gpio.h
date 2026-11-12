// Pinos digitais do microcontrolador.
#pragma once

#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

enum class PinMode : uint8_t { Input, InputPullUp, InputPullDown, Output, OutputOpenDrain };

class IGpio {
 public:
  virtual ~IGpio() = default;
  virtual Status configure(int pin, PinMode mode) = 0;
  virtual Status write(int pin, bool high) = 0;
  virtual Result<bool> read(int pin) = 0;
};

}  // namespace dbmo
