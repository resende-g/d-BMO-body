dbmo_add_library(audio
  SOURCES
    src/audio_subsystem.cpp
    src/audio_types.cpp
    src/tone.cpp
  DEPENDS core config)
