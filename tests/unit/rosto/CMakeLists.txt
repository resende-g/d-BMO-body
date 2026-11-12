foreach(test test_expressions test_face_engine test_face_renderer test_face_subsystem)
  dbmo_add_test(rosto_${test} SOURCES ${test}.cpp LIBS dbmo_rosto)
endforeach()
