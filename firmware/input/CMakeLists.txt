dbmo_add_library(input
  SOURCES
    src/button_processor.cpp
    src/button_sources.cpp
    src/input_subsystem.cpp
    src/mcp23017.cpp
    src/stick_processor.cpp
  DEPENDS core config)
