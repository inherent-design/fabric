# Utils unit tests
target_sources(UnitTests
  PRIVATE
    ErrorHandlingTest.cc
    LoggingTest.cc
    UtilsTest.cc
    CoordinatedGraphTest.cc
)

set_source_files_properties(
  CoordinatedGraphTest.cc
  PROPERTIES
  COMPILE_DEFINITIONS "FABRIC_TEST"
)