# Simulador da EEPROM 24C02, reutilizado pelos testes de integração.
add_library(dbmo_cartridges_sim INTERFACE)
target_include_directories(dbmo_cartridges_sim INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sim)

foreach(test test_eeprom_24c02 test_cartridge_header test_cartridge_subsystem)
  dbmo_add_test(cartridges_${test} SOURCES ${test}.cpp LIBS dbmo_cartridges dbmo_cartridges_sim)
endforeach()
