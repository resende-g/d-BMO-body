#include "dbmo/core/status.h"

#include "dbmo/core/resource_registry.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/core/system.h"

namespace dbmo {

const char *toString(Status status) {
  switch (status) {
    case Status::Ok: return "Ok";
    case Status::InvalidArgument: return "InvalidArgument";
    case Status::OutOfRange: return "OutOfRange";
    case Status::NotInitialized: return "NotInitialized";
    case Status::AlreadyInitialized: return "AlreadyInitialized";
    case Status::NotFound: return "NotFound";
    case Status::Timeout: return "Timeout";
    case Status::Busy: return "Busy";
    case Status::IoError: return "IoError";
    case Status::CrcError: return "CrcError";
    case Status::ProtocolError: return "ProtocolError";
    case Status::Corrupt: return "Corrupt";
    case Status::NoSpace: return "NoSpace";
    case Status::Unsupported: return "Unsupported";
    case Status::NotPermitted: return "NotPermitted";
    case Status::Incompatible: return "Incompatible";
    case Status::HardwareAbsent: return "HardwareAbsent";
    case Status::InternalError: return "InternalError";
  }
  return "Unknown";
}

const char *toString(SubsystemState state) {
  switch (state) {
    case SubsystemState::Uninitialized: return "Uninitialized";
    case SubsystemState::Ready: return "Ready";
    case SubsystemState::Degraded: return "Degraded";
    case SubsystemState::Absent: return "Absent";
    case SubsystemState::Failed: return "Failed";
    case SubsystemState::Stopped: return "Stopped";
  }
  return "Unknown";
}

const char *toString(SystemState state) {
  switch (state) {
    case SystemState::Off: return "Off";
    case SystemState::Booting: return "Booting";
    case SystemState::Running: return "Running";
    case SystemState::Degraded: return "Degraded";
    case SystemState::LowPower: return "LowPower";
    case SystemState::ShuttingDown: return "ShuttingDown";
    case SystemState::Fault: return "Fault";
  }
  return "Unknown";
}

const char *toString(ResourceKind kind) {
  switch (kind) {
    case ResourceKind::Gpio: return "Gpio";
    case ResourceKind::I2cAddress: return "I2cAddress";
    case ResourceKind::SpiChipSelect: return "SpiChipSelect";
    case ResourceKind::Peripheral: return "Peripheral";
  }
  return "Unknown";
}

}  // namespace dbmo
