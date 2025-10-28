#include <Python.h>
#include <gtest/gtest.h>

/**
 * Our tests initialize and finalize the parityos device multiple times. By
 * default this implies that python is also initialized and finalized multiple
 * times (in the same process) when executing the tests. Some python code can
 * tolerate this but this is not always the case. When the python script imports
 * `parityos` (or its dependency `requests`) it seems to be not the case.
 *
 * This environment ensures that the whole python initialization/finalization
 * precodure is only run once. It relies on the fact that our device can detect
 * that a python environment already runs and hence relies on it.
 */
class PythonEnvironment : public ::testing::Environment {
public:
  void SetUp() override { Py_Initialize(); }
  void TearDown() override { Py_Finalize(); }
};

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new PythonEnvironment());
  return RUN_ALL_TESTS();
}
