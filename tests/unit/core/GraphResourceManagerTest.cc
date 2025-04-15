#include "fabric/core/GraphResourceManager.hh"
#include "fabric/utils/Testing.hh"
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace FabricTest {

using namespace Fabric;
using namespace Fabric::Testing;

// Basic test that only checks for successful compilation
TEST(GraphResourceManagerTest, BasicInstance) {
    // Just verify we can get the instance without errors
    EXPECT_NO_THROW({
        auto& manager = GraphResourceManager::instance();
    });
}

} // namespace FabricTest