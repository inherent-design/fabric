#\!/bin/bash

TESTS=(
    "ResourceTest.ResourceCreation"
    "ResourceTest.ResourceLoadUnload"
    "ResourceTest.ResourceHandleBasics"
    "ResourceTest.ResourceHandleLifetime"
    "ResourceTest.ResourceHubGetResource"
    "ResourceTest.ResourceHubCaching"
    "ResourceTest.ResourceHubUnload"
    "ResourceTest.ResourceDirectLoadUnload"
    "ResourceTest.ResourceLifecycleWithRefCounting"
    "ResourceTest.ResourceEviction"
    "ResourceTest.ResourceHubPreloading"
    "ResourceTest.ResourceHubAsyncLoading"
    "ResourceTest.ResourceFactoryRegistration"
    "ResourceTest.ResourceLoadFailure"
    "ResourceTest.ResourceHandleMoveSemantics"
    "ResourceTest.ResourceDependencies"
    "ResourceDeterministicTest.ResourceHubMemoryBudget"
)

echo "Running tests individually to identify problematic tests..."
echo "==============================================================="

FAILED_TESTS=()

for test in "${TESTS[@]}"; do
    echo "Running $test..."
    if ./build/bin/UnitTests --gtest_filter="$test" &> /dev/null; then
        echo "  PASSED ✅"
    else
        echo "  FAILED ❌"
        FAILED_TESTS+=("$test")
    fi
done

echo ""
echo "==============================================================="
echo "Test run completed. Results:"
echo "==============================================================="

if [ ${#FAILED_TESTS[@]} -eq 0 ]; then
    echo "All tests passed successfully\!"
else
    echo "The following tests failed:"
    for failed_test in "${FAILED_TESTS[@]}"; do
        echo "  - $failed_test"
    done
fi
