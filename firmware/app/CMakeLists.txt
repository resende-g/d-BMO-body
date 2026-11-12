# Parte portável da aplicação. src/main.cpp (Arduino) não entra no build nativo.
add_library(dbmo_app_console STATIC src/console.cpp)
target_include_directories(dbmo_app_console PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dbmo_app_console PUBLIC dbmo_core PRIVATE dbmo_options)

# A raiz de composição depende de todos os subsistemas: só existe quando todos entram
# no build (DBMO_SUBSYSTEMS completo).
set(DBMO_APP_DEPENDENCIES
    dbmo_audio dbmo_cartridges dbmo_display dbmo_infrared dbmo_input dbmo_nfc dbmo_power
    dbmo_radio dbmo_rosto dbmo_storage)
set(DBMO_APP_COMPLETE TRUE)
foreach(dependency ${DBMO_APP_DEPENDENCIES})
  if(NOT TARGET ${dependency})
    set(DBMO_APP_COMPLETE FALSE)
  endif()
endforeach()

if(DBMO_APP_COMPLETE)
  add_library(dbmo_app STATIC src/application.cpp src/app_commands.cpp)
  target_include_directories(dbmo_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(dbmo_app
    PUBLIC dbmo_core dbmo_config dbmo_app_console ${DBMO_APP_DEPENDENCIES}
    PRIVATE dbmo_options)
else()
  message(STATUS "dbmo_app fora do build: falta algum subsistema em DBMO_SUBSYSTEMS")
endif()
