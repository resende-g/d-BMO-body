# Simulador do PN532, reutilizado pelos testes de integração.
add_library(dbmo_nfc_sim INTERFACE)
target_include_directories(dbmo_nfc_sim INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sim)
target_link_libraries(dbmo_nfc_sim INTERFACE dbmo_nfc)

foreach(test test_pn532_frame test_pn532 test_nfc_subsystem)
  dbmo_add_test(nfc_${test} SOURCES ${test}.cpp LIBS dbmo_nfc dbmo_nfc_sim)
endforeach()
