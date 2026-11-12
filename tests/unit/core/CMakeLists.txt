foreach(test
    test_config_store
    test_crc
    test_event_bus
    test_log
    test_mock_hal
    test_resource_registry
    test_ring_buffer
    test_state_machine
    test_status
    test_system
    test_version)
  dbmo_add_test(core_${test} SOURCES ${test}.cpp LIBS dbmo_core)
endforeach()
