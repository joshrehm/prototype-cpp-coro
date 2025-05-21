include(StandardOptions)

opt(PROTOTYPE_CPP_CORO_OUTPUT_PATH STRING
    "${PROJECT_OUTPUT_PATH}" 
    "Default output path for compiled Prototype CPP Coro binaries.")

opt(PROTOTYPE_CPP_CORO_WARNINGS_AS_ERRORS BOOL ${WARNINGS_AS_ERRORS}
    "Treat Prototype CPP Coro project compiler warnings as compiler errors")
