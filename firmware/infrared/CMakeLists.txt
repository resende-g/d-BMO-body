dbmo_add_library(infrared
  SOURCES
    src/infrared_subsystem.cpp
    src/ir_codec.cpp
    src/ir_library.cpp
  DEPENDS core config)
