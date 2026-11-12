# Testes de integração: a aplicação inteira (firmware/app) sobre os simuladores de
# hardware usados nos testes de unidade.
if(TARGET dbmo_app)
  dbmo_add_test(integration_test_application
    SOURCES test_application.cpp
    LIBS dbmo_app dbmo_input_sim dbmo_cartridges_sim dbmo_storage_sim dbmo_radio_sim dbmo_nfc_sim)
  target_include_directories(integration_test_application
    PRIVATE ${CMAKE_SOURCE_DIR}/firmware/cartridges/examples)
endif()
