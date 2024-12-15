#include "pch.h"

import scheme_reliability;
using namespace sr;

import std;

#include "CppUnitTest.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::span;
using std::array;
using std::vector;
using std::fabs;
using std::string;
using std::count_if;

namespace sr::tests
{
    TEST_CLASS(SchemeTests)
    {
    private:

        static constexpr size_t processor_count { 4 };
        static constexpr size_t all_count { 8 };

        const SchemeDto<all_count, processor_count> greedy_scheme_dto
        {
            .scheme_name = "simple",
            .elements =
            {
                ElementDto { .name = "c1", .p = 0.8, .q = 0.2 },
                ElementDto { .name = "d1", .p = 0.8, .q = 0.2 },
                ElementDto { .name = "d2", .p = 0.8, .q = 0.2 },
                ElementDto { .name = "c2", .p = 0.8, .q = 0.2 }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "p1", .p = 0.9, .q = 0.1, .normal_load = 40, .max_load = 100,
                    .transitions = { { TrUnit(1, 40) }, { TrUnit(1, 20), TrUnit(2, 10), TrUnit(3, 10) } }
                },
                ProcessorDto
                {
                    .name = "p2", .p = 0.9, .q = 0.1, .normal_load = 20, .max_load = 100,
                    .transitions = { { TrUnit(0, 20) }, { TrUnit(0, 10), TrUnit(2, 10) }, { TrUnit(0, 10), TrUnit(3, 10) } } 
                },
                ProcessorDto
                {
                    .name = "p3", .p = 0.9, .q = 0.1, .normal_load = 30, .max_load = 50,
                    .transitions = { { TrUnit(0, 20), TrUnit(1, 10) }, { TrUnit(0, 10), TrUnit(1, 20) } } 
                },
                ProcessorDto
                {
                    .name = "p4", .p = 0.9, .q = 0.1, .normal_load = 30, .max_load = 50,
                    .transitions = { { TrUnit(0, 20), TrUnit(1, 10) }, { TrUnit(0, 10), TrUnit(1, 20) } } 
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                return sv.all[0] && sv.all[1] && (sv.all[2] || sv.all[3]) && sv.all[4] && (sv.all[5] || sv.all[6]) && sv.all[7];
            },
            .type = SchemeType::Greedy
        };
        
    public:

        TEST_METHOD(calculate_scheme_reliability_brute_force)
        {
            SchemeReliabilitySummaryDto result
            {
                calculate_scheme_reliability<all_count, processor_count>(greedy_scheme_dto)
            };

            Assert::IsTrue(fabs(result.sp - 0.60715008000000004) <= 1e-4);
            Assert::IsTrue(fabs(result.sq - 0.39284992000000019) <= 1e-4);
            Assert::IsTrue(result.sp > result.sq);
            Assert::IsTrue(fabs(result.sp + result.sq - 1.0) <= 1e-5);
            Assert::AreEqual((size_t)256, result.state_vector_set_count);
        }
    };
}