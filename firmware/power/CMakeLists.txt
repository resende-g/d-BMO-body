dbmo_add_library(power
  SOURCES
    src/battery_monitor.cpp
    src/power_hal.cpp
    src/power_subsystem.cpp
  DEPENDS core config)
