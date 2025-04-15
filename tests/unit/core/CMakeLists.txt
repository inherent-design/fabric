# Core unit tests
target_sources(UnitTests
  PRIVATE
    ComponentTest.cc
    ConcurrentGraphTest.cc
    CoreApiTest.cc
    EventTest.cc
    GraphResourceManagerTest.cc
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
    ConcurrentGraphTest.cc
    GraphResourceManagerTest.cc
    ReactiveTest.cc
    ResourceTest.cc
    SpatialTest.cc
    TemporalTest.cc
    PROPERTIES
    COMPILE_DEFINITIONS "FABRIC_TEST"
)