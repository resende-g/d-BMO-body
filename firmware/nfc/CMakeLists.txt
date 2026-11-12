dbmo_add_library(nfc
  SOURCES
    src/nfc_subsystem.cpp
    src/pn532.cpp
    src/pn532_frame.cpp
  DEPENDS core config)
