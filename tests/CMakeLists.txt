find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(
    echonexus_unit_tests
    unit/handler.test.cpp
    unit/serve.test.cpp
    unit/middlewares/cors.test.cpp
    unit/middlewares/logger.test.cpp
    unit/middlewares/router.test.cpp
    unit/types/request.test.cpp
    unit/types/response.test.cpp
    unit/utils/parse.test.cpp
)

target_link_libraries(
    echonexus_unit_tests
    PRIVATE
        EchoNexus
        Catch2::Catch2WithMain
)

target_include_directories(
    echonexus_unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(echonexus_unit_tests PRIVATE cxx_std_23)

catch_discover_tests(
    echonexus_unit_tests
    TEST_PREFIX "unit::"
    PROPERTIES
        LABELS "unit"
)

if(NOT ECHONEXUS_DISABLE_BEAST)
  add_executable(
        echonexus_integration_tests
        integration/serve.test.cpp
        integration/serve_cors.test.cpp
        integration/serve_error_handling.test.cpp
        integration/serve_middleware.test.cpp
        integration/serve_response_types.test.cpp
        integration/serve_routes.test.cpp
    )

  target_link_libraries(
        echonexus_integration_tests
        PRIVATE
            EchoNexus
            Catch2::Catch2WithMain
    )

  target_include_directories(
        echonexus_integration_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

  target_compile_features(echonexus_integration_tests PRIVATE cxx_std_23)

  catch_discover_tests(
        echonexus_integration_tests
        TEST_PREFIX "integration::"
        PROPERTIES
            LABELS "integration"
    )
endif()
