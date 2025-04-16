# Utils unit tests
target_sources(UnitTests
  PRIVATE
  UtilsTest.cc
  ErrorHandlingTest.cc
  LoggingTest.cc
  CoordinatedGraphTest.cc
)

set_source_files_properties(
  UtilsTest.cc
  ErrorHandlingTest.cc
  LoggingTest.cc
  CoordinatedGraphTest.cc
  PROPERTIES
  COMPILE_DEFINITIONS "FABRIC_TEST"
)
