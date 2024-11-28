#include "pch.h"

import sr_reconfiguration_table;

import std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::vector;
using std::array;
using std::span;
using std::count;
using std::string;
using std::format;

namespace sr::tests
{
    TEST_CLASS(ReconfigurationTableTests)
    {
    private:

        string state_to_str(const StateVector& state)
        {
            string result { };
            result.reserve(state.all.size());
            for (auto s : state.all)
                result += (s ? '1' : '0');
            return result;
        }

        void assert_log_s1_s2(const StateVector& s1, const StateVector& s2)
        {
            auto s1_true_count { count(s1.all.begin(), s1.all.end(), true) };
            auto s2_true_count { count(s2.all.begin(), s2.all.end(), true) };

            Logger::WriteMessage("===\n");
            Logger::WriteMessage(format("s1 = {}; true count = {}\n", state_to_str(s1), s1_true_count).c_str());
            Logger::WriteMessage(format("s2 = {}; true count = {}\n", state_to_str(s2), s2_true_count).c_str());

            Assert::IsTrue(s2_true_count >= s1_true_count);
        }

    public:

        TEST_METHOD(reconfiguration_table_4p)
        {
            const size_t all_count { 8 };
            const size_t processor_count { 4 };

            array<double, processor_count> normal_load_values { 40, 20, 30, 30 };
            array<double, processor_count> max_load_values { 100, 100, 50, 50 };

            vector<vector<vector<IdxL>>> table
            {
                { { IdxL(1, 40) }, { IdxL(1, 20), IdxL(2, 10), IdxL(3, 10) } },
                { { IdxL(0, 20) }, { IdxL(0, 10), IdxL(2, 10) }, { IdxL(0, 10), IdxL(3, 10) } },
                { { IdxL(0, 20), IdxL(1, 10) }, { IdxL(0, 10), IdxL(1, 20) } },
                { { IdxL(0, 20), IdxL(1, 10) }, { IdxL(0, 10), IdxL(1, 20) } }
            };

            ReconfigurationTable rt
            {
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table
            };

            StateVectorGenerator sg { all_count, processor_count };

            vector<StateVector> full_state_set { sg.generate_full_2n_state_vector_set() };

            StateVector s2 { Harray<bool>(all_count), processor_count };

            for (auto& s1 : full_state_set)
            {
                s2 = s1;

                rt.reconfigure_state(s1, s2);

                assert_log_s1_s2(s1, s2);
            }
        }
    };
}