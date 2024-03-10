// Placeholder test for the Prototype CPP Coro project. Unit tests use the Catch2 framework:
//
//     https://github.com/catchorg/Catch2/blob/devel/docs/Readme.md
// 
#include<catch2/catch_test_macros.hpp>

TEST_CASE("Captain Placeholder")
{
    REQUIRE(1 + 1 == 2);
    
    SECTION("Test Case Subsection")
    {
        REQUIRE(2 - 1 == 1);
    }
}
