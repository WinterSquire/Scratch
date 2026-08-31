#include "doctest.h"
#include <fstream>
#include <Scratch.hpp>

// template<size_t S>
// struct TestScratchKinetic
// {
//     TestScratchKinetic()
//     {
//         memset(this, 0, sizeof(*this));

//         timestamp[0] = 0;

//         for (int i = 1; i < S; ++i)
//             timestamp[i] = timestamp[i-1] + 60;
//     }

//     constexpr size_t size() { return S; }

//     uint64_t timestamp[S];
//     ScratchResultFrame frames[S];
//     ScratchResultKinetic result;
// };

TEST_SUITE("SerializeScratchTest")
{
    TEST_CASE("TestHTMLSerialize")
    {
        std::ofstream html("test.html");
        ScratchParameterGlobal parameter;

        CHTMLSerializer().serialize(0, NULL, NULL, parameter, html);
    }

    TEST_CASE("TestJSONSerialize")
    {
        // ScratchResult scratchResult{};

        // auto str = toJsonString(scratchResult);

        // CHECK_FALSE(str.isEmpty());
    }

    TEST_CASE("TestJSONDeserialize")
    {
        // ScratchResult scratchResult{};

        // scratchResult.scratchArea.pixel = 1.0;

        // auto json = toJson(scratchResult);

        // scratchResult.scratchArea.pixel = 0.0;

        // fromJson(scratchResult, json);

        // CHECK_EQ(scratchResult.scratchArea.pixel, 1.0);
    }

    TEST_CASE("TestCSVSerialize")
    {
        // TestScratchKinetic<10> testScratchKinetic;

        // std::ofstream file("test.csv");

        // CCSVSerializer::process(
        //     testScratchKinetic.timestamp,
        //     testScratchKinetic.frames,
        //     testScratchKinetic.size(),
        //     testScratchKinetic.result,
        //     file);
    }
}