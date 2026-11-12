dbmo_add_library(radio
  SOURCES
    src/cc1101.cpp
    src/radio_config.cpp
    src/radio_subsystem.cpp
    src/tx_policy.cpp
  DEPENDS core config)
