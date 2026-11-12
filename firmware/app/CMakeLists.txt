# Parte portável da aplicação. src/main.cpp (Arduino) não entra no build nativo.
add_library(dbmo_app_console STATIC src/console.cpp)
target_include_directories(dbmo_app_console PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dbmo_app_console PUBLIC dbmo_core PRIVATE dbmo_options)
