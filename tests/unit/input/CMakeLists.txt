# Simulador do MCP23017, reutilizado pelos testes de integração.
add_library(dbmo_input_sim INTERFACE)
target_include_directories(dbmo_input_sim INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sim)

foreach(test test_button_processor test_mcp23017 test_stick_processor test_input_subsystem)
  dbmo_add_test(input_${test} SOURCES ${test}.cpp LIBS dbmo_input dbmo_input_sim)
endforeach()
