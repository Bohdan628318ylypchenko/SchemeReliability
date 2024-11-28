export module sr_research;

import sr_scheme;

import std;

using std::span;
using std::array;
using std::vector;
using std::string;
using std::print;

export namespace sr::research
{
    void simple();
}

module : private;

namespace sr::research
{
    void simple()
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

        array<double, all_count> p_values { 0.9, 0.9, 0.9, 0.9, 0.8, 0.8, 0.8, 0.8 };
        array<double, all_count> q_values { 0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2 };

        array<string, all_count> element_names { "p1", "p2", "p3", "p4", "c1", "d1", "d2", "c2" };

        Scheme scheme
        {
            .sfunc = [](const StateVector& sv)
            {
                return sv.all[0] && sv.all[1] && (sv.all[2] || sv.all[3]) && sv.all[4] && (sv.all[5] || sv.all[6]) && sv.all[7];
            },
            .p = Harray<double> { p_values },
            .q = Harray<double> { q_values },
            .element_names = Harray<string> { element_names },
            .rt = ReconfigurationTable
            {
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table
            }
        };

        SchemeReliability result { calculate_reliability(scheme) };

        print("{}\n", result.represent());
    }
}
