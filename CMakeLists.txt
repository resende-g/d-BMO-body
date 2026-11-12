# Build nativo (computador) do firmware próprio do d-BMO: bibliotecas dos subsistemas e
# testes com Unity. O build para o ESP32-S3 é feito pelo PlatformIO (platformio.ini).
cmake_minimum_required(VERSION 3.16)
project(dbmo_firmware VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DBMO_BUILD_TESTS "Compila os testes nativos" ON)
option(DBMO_WERROR "Trata avisos como erro" ON)
option(DBMO_SANITIZE "Compila com AddressSanitizer e UndefinedBehaviorSanitizer" OFF)
option(DBMO_COVERAGE "Instrumenta para cobertura (gcov)" OFF)
set(DBMO_SUBSYSTEMS
    "config;core;display;input;audio;radio;nfc;infrared;storage;power;cartridges;rosto;app"
    CACHE STRING "Subsistemas a compilar (lista separada por ';')")

# Opções de compilação comuns. Sem exceções e sem RTTI, como no build do ESP32.
add_library(dbmo_options INTERFACE)
target_compile_options(dbmo_options INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wformat=2
  $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor -fno-exceptions -fno-rtti>
  $<$<BOOL:${DBMO_WERROR}>:-Werror>)
if(DBMO_SANITIZE)
  target_compile_options(dbmo_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(dbmo_options INTERFACE -fsanitize=address,undefined)
endif()
if(DBMO_COVERAGE)
  target_compile_options(dbmo_options INTERFACE --coverage -O0)
  target_link_options(dbmo_options INTERFACE --coverage)
endif()

# dbmo_add_library(<nome> [SOURCES ...] [DEPENDS <outros subsistemas>])
# Cria a biblioteca dbmo_<nome> com os cabeçalhos em include/. Sem SOURCES, a
# biblioteca é só de cabeçalhos.
function(dbmo_add_library name)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})
  set(deps "")
  foreach(dep ${ARG_DEPENDS})
    list(APPEND deps dbmo_${dep})
  endforeach()
  if(ARG_SOURCES)
    add_library(dbmo_${name} STATIC ${ARG_SOURCES})
    target_include_directories(dbmo_${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(dbmo_${name} PUBLIC ${deps} PRIVATE dbmo_options)
  else()
    add_library(dbmo_${name} INTERFACE)
    target_include_directories(dbmo_${name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(dbmo_${name} INTERFACE ${deps})
  endif()
endfunction()

foreach(subsystem ${DBMO_SUBSYSTEMS})
  if(EXISTS ${CMAKE_SOURCE_DIR}/firmware/${subsystem}/CMakeLists.txt)
    add_subdirectory(firmware/${subsystem})
  endif()
endforeach()

if(DBMO_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
