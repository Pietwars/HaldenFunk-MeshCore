"""Host checks live in the project root tests/; no embedded Unity dependency."""
from platformio.test.runners.base import TestRunnerBase

class CustomTestRunner(TestRunnerBase):
    EXTRA_LIB_DEPS = []

    def stage_testing(self):
        raise RuntimeError("Use the HaldenFunk-MeshCore root tests/ scripts; embedded tests are not implemented.")
