# Testes nativos com Unity (tests/third_party/unity, v2.6.1, MIT).

add_library(dbmo_unity STATIC third_party/unity/unity.c)
target_include_directories(dbmo_unity PUBLIC third_party/unity)

# Mocks de HAL (só cabeçalhos) e macros de apoio aos testes.
add_library(dbmo_test_support INTERFACE)
target_include_directories(dbmo_test_support INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/include
  ${CMAKE_CURRENT_SOURCE_DIR}/support)
target_link_libraries(dbmo_test_support INTERFACE dbmo_unity dbmo_core)

# dbmo_add_test(<nome> SOURCES ... [LIBS ...])
# Um executável por arquivo de teste; cada um tem o seu main() com UNITY_BEGIN/END.
function(dbmo_add_test name)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
  add_executable(${name} ${ARG_SOURCES})
  target_link_libraries(${name} PRIVATE ${ARG_LIBS} dbmo_test_support dbmo_options)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

foreach(subsystem ${DBMO_SUBSYSTEMS})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/${subsystem}/CMakeLists.txt)
    add_subdirectory(unit/${subsystem})
  endif()
endforeach()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt)
  add_subdirectory(integration)
endif()
