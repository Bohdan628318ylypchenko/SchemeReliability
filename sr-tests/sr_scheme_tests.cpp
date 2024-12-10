#include "pch.h"

#include "CppUnitTest.h"

import sr_scheme;

import std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::span;
using std::array;
using std::vector;
using std::fabs;
using std::string;

namespace sr::tests
{
    TEST_CLASS(SchemeTests)
    {
    private:
        
        static const size_t ALL_COUNT { 8 };
        static const size_t PROCESSOR_COUNT { 4 };

        array<double, ALL_COUNT> normal_load_values { 40, 20, 30, 30 };
        array<double, PROCESSOR_COUNT> max_load_values { 100, 100, 50, 50 };

        vector<TransitionSet> table
        {
            { { IdxL(1, 40) }, { IdxL(1, 20), IdxL(2, 10), IdxL(3, 10) } },
            { { IdxL(0, 20) }, { IdxL(0, 10), IdxL(2, 10) }, { IdxL(0, 10), IdxL(3, 10) } },
            { { IdxL(0, 20), IdxL(1, 10) }, { IdxL(0, 10), IdxL(1, 20) } },
            { { IdxL(0, 20), IdxL(1, 10) }, { IdxL(0, 10), IdxL(1, 20) } }
        };
        
        array<double, ALL_COUNT> p_values { 0.9, 0.9, 0.9, 0.9, 0.8, 0.8, 0.8, 0.8 };
        array<double, ALL_COUNT> q_values { 0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2 };
        array<string, ALL_COUNT> element_names { "p1", "p2", "p3", "p4", "c1", "d1", "d2", "c2" };

        SFunc sfunc = [](const StateVector& sv)
        {
            return sv.all[0] && sv.all[1] && (sv.all[2] || sv.all[3]) && sv.all[4] && (sv.all[5] || sv.all[6]) && sv.all[7];
        };

    public:

        TEST_METHOD(calculate_scheme_reliability_brute_force)
        {
            Scheme scheme
            {
                sfunc,
                span<double> { p_values },
                span<double> { q_values },
                span<string> { element_names },
                new BruteForceReconfigurationTable
                {
                    PROCESSOR_COUNT,
                    span<double> { normal_load_values },
                    span<double> { max_load_values },
                    table,
                    sfunc
                }
            };

            SchemeReliability result { calculate_reliability(scheme) };

            Assert::AreEqual(static_cast<size_t>(256), result.scored_state_set.size());
            Assert::IsTrue(fabs(result.sp - 0.6144) <= 1e-4);
            Assert::IsTrue(fabs(result.sq - 0.3856) <= 1e-4);
            Assert::IsTrue(result.sp > result.sq);
            Assert::IsTrue(fabs(result.sp + result.sq - 1.0) <= 1e-5);
        }

        TEST_METHOD(calculate_scheme_reliability_greedy)
        {
            Scheme scheme
            {
                sfunc,
                span<double> { p_values },
                span<double> { q_values },
                span<string> { element_names },
                new GreedyReconfigurationTable
                {
                    PROCESSOR_COUNT,
                    span<double> { normal_load_values },
                    span<double> { max_load_values },
                    table
                }
            };

            SchemeReliability result { calculate_reliability(scheme) };

            Assert::AreEqual(static_cast<size_t>(256), result.scored_state_set.size());
            Assert::IsTrue(fabs(result.sp - 0.6144) <= 1e-4);
            Assert::IsTrue(fabs(result.sq - 0.3856) <= 1e-4);
            Assert::IsTrue(result.sp > result.sq);
            Assert::IsTrue(fabs(result.sp + result.sq - 1.0) <= 1e-5);
        }
    };
}