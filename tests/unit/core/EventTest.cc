#include "fabric/core/Event.hh"
#include "fabric/utils/Testing.hh"
#include "fabric/utils/ErrorHandling.hh"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace Fabric;
using namespace Fabric::Testing;

class EventTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test events
    testEvent1 = std::make_unique<Event>("click", "button1");
    testEvent2 = std::make_unique<Event>("input", "textfield1");
    dispatcher = std::make_unique<EventDispatcher>();
    recorder = std::make_unique<EventRecorder>();
  }

  std::unique_ptr<Event> testEvent1;
  std::unique_ptr<Event> testEvent2;
  std::unique_ptr<EventDispatcher> dispatcher;
  std::unique_ptr<EventRecorder> recorder;
};

TEST_F(EventTest, ConstructorThrowsOnEmptyType) {
  EXPECT_THROW(Event("", "source"), FabricException);
}

TEST_F(EventTest, GetType) {
  EXPECT_EQ(testEvent1->getType(), "click");
  EXPECT_EQ(testEvent2->getType(), "input");
}

TEST_F(EventTest, GetSource) {
  EXPECT_EQ(testEvent1->getSource(), "button1");
  EXPECT_EQ(testEvent2->getSource(), "textfield1");
}

TEST_F(EventTest, SetGetData) {
  testEvent1->setData<int>("intData", 42);
  testEvent1->setData<float>("floatData", 3.14f);
  testEvent1->setData<std::string>("stringData", "hello");
  testEvent1->setData<bool>("boolData", true);

  EXPECT_EQ(testEvent1->getData<int>("intData"), 42);
  EXPECT_FLOAT_EQ(testEvent1->getData<float>("floatData"), 3.14f);
  EXPECT_EQ(testEvent1->getData<std::string>("stringData"), "hello");
  EXPECT_EQ(testEvent1->getData<bool>("boolData"), true);
}

TEST_F(EventTest, GetDataThrowsOnMissingKey) {
  EXPECT_THROW(testEvent1->getData<int>("nonexistent"), FabricException);
}

TEST_F(EventTest, GetDataThrowsOnWrongType) {
  testEvent1->setData<int>("intData", 42);
  EXPECT_THROW(testEvent1->getData<std::string>("intData"), FabricException);
}

TEST_F(EventTest, HandledFlag) {
  EXPECT_FALSE(testEvent1->isHandled());
  
  testEvent1->setHandled(true);
  EXPECT_TRUE(testEvent1->isHandled());
  
  testEvent1->setHandled(false);
  EXPECT_FALSE(testEvent1->isHandled());
}

TEST_F(EventTest, PropagateFlag) {
  EXPECT_TRUE(testEvent1->shouldPropagate());
  
  testEvent1->setPropagate(false);
  EXPECT_FALSE(testEvent1->shouldPropagate());
  
  testEvent1->setPropagate(true);
  EXPECT_TRUE(testEvent1->shouldPropagate());
}

TEST_F(EventTest, AddEventListener) {
  std::string handlerId = dispatcher->addEventListener("click", recorder->getHandler());
  EXPECT_FALSE(handlerId.empty());
}

TEST_F(EventTest, AddEventListenerThrowsOnEmptyType) {
  EXPECT_THROW(dispatcher->addEventListener("", recorder->getHandler()), FabricException);
}

TEST_F(EventTest, AddEventListenerThrowsOnNullHandler) {
  EXPECT_THROW(dispatcher->addEventListener("click", nullptr), FabricException);
}

TEST_F(EventTest, RemoveEventListener) {
  std::string handlerId = dispatcher->addEventListener("click", recorder->getHandler());
  EXPECT_TRUE(dispatcher->removeEventListener("click", handlerId));
  EXPECT_FALSE(dispatcher->removeEventListener("click", handlerId)); // Already removed
  EXPECT_FALSE(dispatcher->removeEventListener("nonexistent", "invalid"));
}

TEST_F(EventTest, DispatchEvent) {
  // Create listener that doesn't mark the event as handled
  dispatcher->addEventListener("click", [this](const Event& event) {
    recorder->recordEvent(event);
  });
  
  EXPECT_FALSE(dispatcher->dispatchEvent(*testEvent1)); // Returns false because event not marked as handled
  EXPECT_EQ(recorder->eventCount, 1);
  EXPECT_EQ(recorder->lastEventType, "click");
  EXPECT_EQ(recorder->lastEventSource, "button1");
  
  EXPECT_FALSE(dispatcher->dispatchEvent(*testEvent2)); // No listeners for "input"
  EXPECT_EQ(recorder->eventCount, 1); // Should not have changed
}

TEST_F(EventTest, EventHandling) {
  dispatcher->addEventListener("click", [](const Event& event) {
    const_cast<Event&>(event).setHandled(true);
  });
  
  EXPECT_TRUE(dispatcher->dispatchEvent(*testEvent1));
  EXPECT_TRUE(testEvent1->isHandled());
}

TEST_F(EventTest, MultipleEventListeners) {
  int handler1Calls = 0;
  int handler2Calls = 0;
  
  dispatcher->addEventListener("click", [&handler1Calls](const Event& event) {
    handler1Calls++;
  });
  
  dispatcher->addEventListener("click", [&handler2Calls](const Event& event) {
    handler2Calls++;
    const_cast<Event&>(event).setHandled(true); // Should stop propagation
  });
  
  dispatcher->addEventListener("click", [](const Event& event) {
    // This should not be called because event is handled by previous listener
    FAIL() << "This handler should not be called";
  });
  
  EXPECT_TRUE(dispatcher->dispatchEvent(*testEvent1));
  EXPECT_EQ(handler1Calls, 1);
  EXPECT_EQ(handler2Calls, 1);
}

