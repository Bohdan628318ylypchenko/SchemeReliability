#include "pch.h"

import scheme_reliability;

import std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace sr::tests
{
    TEST_CLASS(ReconfigurationTableTests)
    {
        /*
    private:

        static const size_t ALL_COUNT_4P { 8 };
        static const size_t PROCESSOR_COUNT_4P { 4 };

        array<double, PROCESSOR_COUNT_4P> normal_load_values_4p { 40, 20, 30, 30 };
        array<double, PROCESSOR_COUNT_4P> max_load_values_4p { 100, 100, 50, 50 };
        
        vector<TransitionSet> table_4p
        {
            { { IdxL(1, 40) }, { IdxL(1, 20), IdxL(2, 10), IdxL(3, 10) } },
            { { IdxL(0, 20) }, { IdxL(0, 10), IdxL(2, 10) }, { IdxL(0, 10), IdxL(3, 10) } },
            { { IdxL(0, 20), IdxL(1, 10) }, { IdxL(0, 10), IdxL(1, 20) } },
            { { IdxL(0, 20), IdxL(1, 10) }, { IdxL(0, 10), IdxL(1, 20) } }
        };

        SFunc sfunc_4p = [](const StateVector& sv)
        {
            auto& s = sv.all;
            return s[0] && s[1] && (s[2] || s[3]);
        };

        static const size_t ALL_COUNT_5P { 8 };
        static const size_t PROCESSOR_COUNT_5P { 5 };

        array<double, PROCESSOR_COUNT_5P> normal_load_values_5p { 50, 50, 50, 30, 30 };
        array<double, PROCESSOR_COUNT_5P> max_load_values_5p { 80, 80, 80, 60, 60 };

        vector<TransitionSet> table_5p
        {
            { { IdxL { 1, 25 }, IdxL { 2, 25 } } },
            { { IdxL { 0, 25 }, IdxL { 2, 25 } } },
            { { IdxL { 0, 25 }, IdxL { 1, 25 } } },
            { },
            { }
        };

        SFunc sfunc_5p = [](const StateVector& sv)
        {
            auto& s = sv.all;
            return s[0] && s[1] && s[2] && s[3] && s[4] && s[5];
        };

        string state_to_str(const StateVector& state)
        {
            string result { };
            result.reserve(state.all.size());
            for (auto s : state.all)
                result += (s ? '1' : '0');
            return result;
        }

        void assert_log_s1_s2(const StateVector& s1, const StateVector& s2, function<void(const size_t, const size_t)> true_count_action)
        {
            auto s1_true_count { count(s1.all.begin(), s1.all.end(), true) };
            auto s2_true_count { count(s2.all.begin(), s2.all.end(), true) };

            Logger::WriteMessage("===\n");
            Logger::WriteMessage(format("s1 = {}; true count = {}\n", state_to_str(s1), s1_true_count).c_str());
            Logger::WriteMessage(format("s2 = {}; true count = {}\n", state_to_str(s2), s2_true_count).c_str());

            true_count_action(s1_true_count, s2_true_count);
        }

    public:

        TEST_METHOD(reconfiguration_table_brute_force_4p)
        {
            unique_ptr<ReconfigurationTable> rt
            {
                new BruteForceReconfigurationTable
                {
                    PROCESSOR_COUNT_4P,
                    span<double> { normal_load_values_4p },
                    span<double> { max_load_values_4p },
                    table_4p,
                    sfunc_4p
                }
            };

            StateVectorGenerator sg { ALL_COUNT_4P, PROCESSOR_COUNT_4P };

            vector<StateVector> full_state_set { sg.generate_full_2n_state_vector_set() };

            for (auto& s1 : full_state_set)
            {
                StateVector s2 = rt->reconfigure_state(s1);

                assert_log_s1_s2(
                    s1, s2,
                    [](const auto s1_true_count, const auto s2_true_count)
                    {
                        Assert::IsTrue(s1_true_count <= s2_true_count);
                    }
                );
            }
        }

        TEST_METHOD(reconfiguration_table_greedy_4p)
        {
            unique_ptr<ReconfigurationTable> rt
            {
                new GreedyReconfigurationTable
                {
                    PROCESSOR_COUNT_4P,
                    span<double> { normal_load_values_4p },
                    span<double> { max_load_values_4p },
                    table_4p
                }
            };

            StateVectorGenerator sg { ALL_COUNT_4P, PROCESSOR_COUNT_4P };

            vector<StateVector> full_state_set { sg.generate_full_2n_state_vector_set() };

            for (auto& s1 : full_state_set)
            {
                StateVector s2 = rt->reconfigure_state(s1);

                assert_log_s1_s2(
                    s1, s2,
                    [](const auto s1_true_count, const auto s2_true_count)
                    {
                        Assert::IsTrue(s1_true_count <= s2_true_count);
                    }
                );
            }
        }

        TEST_METHOD(reconfiguration_table_brute_force_5p)
        {
            unique_ptr<ReconfigurationTable> rt
            {
                new BruteForceReconfigurationTable
                {
                    PROCESSOR_COUNT_5P,
                    span<double> { normal_load_values_5p },
                    span<double> { max_load_values_5p },
                    table_5p,
                    sfunc_5p
                }
            };

            StateVectorGenerator sg { ALL_COUNT_5P, PROCESSOR_COUNT_5P };

            vector<StateVector> full_state_set { sg.generate_full_2n_state_vector_set() };

            for (auto& s1 : full_state_set)
            {
                StateVector s2 = rt->reconfigure_state(s1);

                assert_log_s1_s2(s1, s2, [](const auto a, const auto b) { return; });
            }
        }

        TEST_METHOD(reconfiguration_table_greedy_5p)
        {
            unique_ptr<ReconfigurationTable> rt
            {
                new GreedyReconfigurationTable
                {
                    PROCESSOR_COUNT_5P,
                    span<double> { normal_load_values_5p },
                    span<double> { max_load_values_5p },
                    table_5p
                }
            };

            StateVectorGenerator sg { ALL_COUNT_5P, PROCESSOR_COUNT_5P };

            vector<StateVector> full_state_set { sg.generate_full_2n_state_vector_set() };

            for (auto& s1 : full_state_set)
            {
                StateVector s2 = rt->reconfigure_state(s1);

                assert_log_s1_s2(s1, s2, [](const auto a, const auto b) { return; });
            }
        }
        */
    };
}