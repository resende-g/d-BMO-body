# Simulador do CC1101, reutilizado pelos testes de integração.
add_library(dbmo_radio_sim INTERFACE)
target_include_directories(dbmo_radio_sim INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sim)
target_link_libraries(dbmo_radio_sim INTERFACE dbmo_radio)

foreach(test test_radio_config test_tx_policy test_cc1101 test_radio_subsystem)
  dbmo_add_test(radio_${test} SOURCES ${test}.cpp LIBS dbmo_radio dbmo_radio_sim)
endforeach()
