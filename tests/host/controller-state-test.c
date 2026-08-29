#include "unity.h"

void setUp() {} 
void tearDown() {} 


void must_assert_true() {
    TEST_ASSERT_TRUE(1);
}


int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(must_assert_true);
  return UNITY_END();
}
