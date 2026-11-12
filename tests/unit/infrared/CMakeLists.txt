foreach(test test_ir_codec test_ir_library test_infrared_subsystem)
  dbmo_add_test(infrared_${test} SOURCES ${test}.cpp LIBS dbmo_infrared)
endforeach()
