#include "doctest.h"

#include <Scratch.hpp>

TEST_SUITE("SerializeScratchTest")
{
    TEST_CASE("TestSerialize")
    {
        ScratchResult scratchResult{};

        auto str = toJsonString(scratchResult);

        CHECK_FALSE(str.isEmpty());
    }

    TEST_CASE("TestDeserialize")
    {
        ScratchResult scratchResult{};

        scratchResult.area.pixel = 1.0;

        auto json = toJson(scratchResult);

        scratchResult.area.pixel = 0.0;

        fromJson(scratchResult, json);

        CHECK_EQ(scratchResult.area.pixel, 1.0);
    }
}