dbmo_add_library(storage
  SOURCES
    src/file_config_backend.cpp
    src/file_log_sink.cpp
    src/file_utils.cpp
    src/path.cpp
    src/storage_subsystem.cpp
  DEPENDS core config)
