#include <unity.h>

#include "GestureProcessor.h"

void test_deadband_returns_hover() {
  GestureProcessor processor(6.0f, 30.0f);

  const GestureCommand command = processor.process(GestureAttitude{3.0f, -4.0f});

  TEST_ASSERT_TRUE(command.neutral);
  TEST_ASSERT_EQUAL_INT8(0, command.pitch);
  TEST_ASSERT_EQUAL_INT8(0, command.roll);
  TEST_ASSERT_EQUAL_STRING("HOVER", gestureDirectionName(command.direction));
}

void test_forward_pitch_normalizes() {
  GestureProcessor processor(6.0f, 30.0f);

  const GestureCommand command = processor.process(GestureAttitude{15.0f, 0.0f});

  TEST_ASSERT_FALSE(command.neutral);
  TEST_ASSERT_EQUAL_INT8(50, command.pitch);
  TEST_ASSERT_EQUAL_INT8(0, command.roll);
  TEST_ASSERT_EQUAL_STRING("FORWARD", gestureDirectionName(command.direction));
}

void test_roll_dominates_direction() {
  GestureProcessor processor(6.0f, 30.0f);

  const GestureCommand command = processor.process(GestureAttitude{8.0f, -24.0f});

  TEST_ASSERT_FALSE(command.neutral);
  TEST_ASSERT_EQUAL_INT8(27, command.pitch);
  TEST_ASSERT_EQUAL_INT8(-80, command.roll);
  TEST_ASSERT_EQUAL_STRING("LEFT", gestureDirectionName(command.direction));
}

void test_values_clamp_to_range() {
  GestureProcessor processor(6.0f, 30.0f);

  const GestureCommand command = processor.process(GestureAttitude{90.0f, -90.0f});

  TEST_ASSERT_EQUAL_INT8(100, command.pitch);
  TEST_ASSERT_EQUAL_INT8(-100, command.roll);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_deadband_returns_hover);
  RUN_TEST(test_forward_pitch_normalizes);
  RUN_TEST(test_roll_dominates_direction);
  RUN_TEST(test_values_clamp_to_range);
  return UNITY_END();
}

