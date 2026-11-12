# Sistema de arquivos em memória, reutilizado pelos testes de integração.
add_library(dbmo_storage_sim INTERFACE)
target_include_directories(dbmo_storage_sim INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sim)
target_link_libraries(dbmo_storage_sim INTERFACE dbmo_storage)

foreach(test test_path test_file_utils test_file_log_sink test_storage_subsystem)
  dbmo_add_test(storage_${test} SOURCES ${test}.cpp LIBS dbmo_storage dbmo_storage_sim)
endforeach()
