dbmo_add_test(app_test_console SOURCES test_console.cpp LIBS dbmo_app_console)
if(TARGET dbmo_app_menu)
  dbmo_add_test(app_test_menu SOURCES test_menu.cpp LIBS dbmo_app_menu)
endif()
