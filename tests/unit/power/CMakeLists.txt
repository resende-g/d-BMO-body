foreach(test test_battery_monitor test_power_subsystem)
  dbmo_add_test(power_${test} SOURCES ${test}.cpp LIBS dbmo_power)
endforeach()
