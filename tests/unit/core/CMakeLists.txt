# Core unit tests
target_sources(UnitTests
  PRIVATE
  ComponentTest.cc
  CoreApiTest.cc
  EventTest.cc
  ResourceHubTest.cc
  LifecycleTest.cc
  PluginTest.cc
  CommandTest.cc
  ReactiveTest.cc
  ResourceTest.cc
  SpatialTest.cc
  TemporalTest.cc
)

set_source_files_properties(
  CommandTest.cc
  LifecycleTest.cc
  PluginTest.cc
  ResourceHubTest.cc
  ReactiveTest.cc
  ResourceTest.cc
  SpatialTest.cc
  TemporalTest.cc
  PROPERTIES
  COMPILE_DEFINITIONS "FABRIC_TEST"
)
