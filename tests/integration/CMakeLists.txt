# Integration tests
target_sources(IntegrationTests
  PRIVATE
    ComponentLifecycleIntegrationTest.cc
    ParserLoggingIntegrationTest.cc
    QuantumFluctuationIntegrationTest.cc
)

set_source_files_properties(
    QuantumFluctuationIntegrationTest.cc
    PROPERTIES
    COMPILE_DEFINITIONS "FABRIC_TEST"
)