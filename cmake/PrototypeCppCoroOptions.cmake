include(StandardOptions)

opt(PROTOTYPE_CPP_CORO_OUTPUT_PATH STRING
    "${PROJECT_OUTPUT_PATH}" 
    "Default output path for compiled Prototype CPP Coro binaries.")

opt(PROTOTYPE_CPP_CORO_ENABLE_TESTS BOOL OFF
    "Build Prototype CPP Coro unit tests")

opt(PROTOTYPE_CPP_CORO_RUN_TESTS_ON_BUILD BOOL OFF
    "Run Prototype CPP Coro unit tests on build")

opt(PROTOTYPE_CPP_CORO_ENABLE_CPPCHECK BOOL ON
    "Enable CppCheck static code analysis for Prototype CPP Coro")

opt(PROTOTYPE_CPP_CORO_WARNINGS_AS_ERRORS BOOL ${WARNINGS_AS_ERRORS}
    "Treat Prototype CPP Coro project compiler warnings as compiler errors")
