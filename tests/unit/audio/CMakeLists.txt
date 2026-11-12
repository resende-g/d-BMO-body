foreach(test test_audio_types test_tone test_audio_subsystem)
  dbmo_add_test(audio_${test} SOURCES ${test}.cpp LIBS dbmo_audio)
endforeach()
