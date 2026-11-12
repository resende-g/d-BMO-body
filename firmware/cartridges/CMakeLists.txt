dbmo_add_library(cartridges
  SOURCES
    src/cartridge_header.cpp
    src/cartridge_subsystem.cpp
    src/eeprom_24c02.cpp
  DEPENDS core config)
# Drivers de exemplo (só cabeçalhos).
target_include_directories(dbmo_cartridges PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/examples)

# Ferramenta de computador: gera e confere a imagem da EEPROM de um cartucho.
add_executable(dbmo_cartridge_image tools/cartridge_image.cpp)
target_link_libraries(dbmo_cartridge_image PRIVATE dbmo_cartridges dbmo_options)
