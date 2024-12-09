export module sr_research;

import sr_scheme;

import std;

using std::span;
using std::array;
using std::vector;
using std::string;
using std::format;
using std::cerr;
using std::println;
using std::unique_ptr;
using std::ofstream;
using std::function;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::seconds;

export namespace sr::research
{
    void simple();
    void v17_original();
    void v17_rt();
}

module : private;

namespace sr::research
{
    class Utils
    {
    public:

        Utils() = delete;

        static void dump_text_summary(const SchemeReliability& sr)
        {
            println("sp = {}, sq = {}", sr.sp, sr.sq);
            println("state count = {}", sr.scored_state_set.size());
        }

        static void dump_binary_scored_state_set(
            const vector<ScoredStateVector>& scored_state_set,
            const string& dump_file_name
        ) {
            ofstream file { dump_file_name, std::ios::binary };
            if (!file.is_open())
            {
                print(cerr, "Error: can't open file {} for dumping scored state set\n", dump_file_name);
                return;
            }

            const size_t bufsize { 8768 };
            unique_ptr<char[]> buffer { new char[bufsize] };
            file.rdbuf()->pubsetbuf(buffer.get(), bufsize);

            for (const ScoredStateVector& ssv : scored_state_set)
            {
                file.write(reinterpret_cast<const char*>(&ssv.scheme_state_sv1), sizeof(bool));
                file.write(reinterpret_cast<const char*>(&ssv.scheme_state_sv2), sizeof(bool));
                file.write(reinterpret_cast<const char*>(&ssv.scheme_state), sizeof(bool));
                file.write(reinterpret_cast<const char*>(&ssv.probability), sizeof(double));
                file.write(reinterpret_cast<const char*>(ssv.sv1.sv.get_data()), ssv.sv1.all.size() * sizeof(bool));
                file.write(reinterpret_cast<const char*>(ssv.sv2.sv.get_data()), ssv.sv2.all.size() * sizeof(bool));
            }

            file.flush();

            println("saved scored state set");
        }

        template<typename T>
        static T execution_time(function<T(void)> action)
        {
            auto start = high_resolution_clock::now();
            T result { action() };
            auto end = high_resolution_clock::now();
        
            auto duration = duration_cast<seconds>(end - start);
        
            println("time = {}", duration.count());
            
            return result;
        }
    };

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

        auto sfunc = [](const StateVector& sv)
        {
            return sv.all[0] && sv.all[1] && (sv.all[2] || sv.all[3]) && sv.all[4] && (sv.all[5] || sv.all[6]) && sv.all[7];
        };
        Scheme scheme
        {
            .sfunc = sfunc,
            .p = Harray<double> { p_values },
            .q = Harray<double> { q_values },
            .element_names = Harray<string> { element_names },
            .rt = ReconfigurationTable
            {
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table,
                sfunc
            }
        };

        auto result
        {
            Utils::execution_time<SchemeReliability>(
                [&scheme]() { return calculate_reliability(scheme); }
            )
        };

        Utils::dump_text_summary(result);
        Utils::dump_binary_scored_state_set(result.scored_state_set, "simple-scored-state-set.sstd");
    }

    void v17_original()
    {
        const size_t all_count { 23 };
        const size_t processor_count { 5 };

        array<double, processor_count> normal_load_values { 50, 50, 50, 30, 30 };
        array<double, processor_count> max_load_values { 80, 80, 80, 60, 60 };

        vector<vector<vector<IdxL>>> table
        {
            { { IdxL { 1, 25 }, IdxL { 2, 25 } } },
            { { IdxL { 0, 25 }, IdxL { 2, 25 } } },
            { { IdxL { 0, 25 }, IdxL { 1, 25 } } },
            { },
            { }
        };

        array<string, all_count> element_names
        {
            "pr1" /* 0 */, "pr2" /* 1 */, "pr3" /* 2 */, "pr5" /* 3 */, "pr6" /* 4 */,
            "a1" /* 5 */, "a2" /* 6 */,
            "b1" /* 7 */, "b2" /* 8 */, "b4" /* 9 */, "b5" /* 10 */,
            "c1", /* 11 */ "c2" /* 12 */, "c4" /* 13 */, "c5" /* 14 */, "c6" /* 15 */,
            "d1", /* 16 */ "d2" /* 17 */, "d3" /* 18 */, "d6" /* 19 */, "d8" /* 20 */,
            "m1" /* 21 */, "m2" /* 22 */
        };

        const double qpr { 1.2e-4 };
        const double qpa { 1.2e-4 };
        const double qpb { 1.5e-5 };
        const double qpc { 4.1e-4 };
        const double qpd { 2.2e-5 };
        const double qpm { 3.6e-4 };

        array<double, all_count> q_values
        {
            qpr, qpr, qpr, qpr, qpr,
            qpa, qpa,
            qpb, qpb, qpb, qpb,
            qpc, qpc, qpc, qpc, qpc,
            qpd, qpd, qpd, qpd, qpd,
            qpm, qpm
        };

        array<double, all_count> p_values { };
        for (size_t i = 0; i < all_count; i++)
            p_values[i] = 1.0 - q_values[i];

        auto sfunc = [](const StateVector& sv)
        {
            span<bool> s = sv.all;

            bool f1 = (s[16] + s[17]) * s[11] * (s[17] + s[18]) * s[12] * (s[7] + s[8]);
            bool f2 = s[19] * s[13] * s[8];
            bool f3 = s[0] * s[1] * s[2];
            bool f4 = s[20] * (s[14] + s[15]) * s[9];
            bool f5 = s[3] * s[4] * s[10];
            bool f6 = s[5] * s[6] * (s[21] + s[22]);

            return f1 * f2 * f3 * f4 * f5 * f6;
        };
        Scheme scheme
        {
            .sfunc = sfunc,
            .p = Harray<double> { p_values },
            .q = Harray<double> { q_values },
            .element_names = Harray<string> { element_names },
            .rt = ReconfigurationTable
            {
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table,
                sfunc
            }
        };

        auto result
        {
            Utils::execution_time<SchemeReliability>(
                [&scheme]() { return calculate_reliability(scheme); }
            )
        };

        Utils::dump_text_summary(result);
        Utils::dump_binary_scored_state_set(result.scored_state_set, "v17_original-scored-state-set.sstd");
    }

    void v17_rt()
    {
        const size_t all_count { 23 };
        const size_t processor_count { 5 };

        array<double, processor_count> normal_load_values { 50, 50, 50, 30, 30 };
        array<double, processor_count> max_load_values { 100, 100, 100, 60, 60 };

        //vector<vector<vector<IdxL>>> table
        //{
        //    { { IdxL { 1, 40 }, IdxL { 2, 10 } }, { IdxL { 1, 30 }, IdxL { 2, 20 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 1, 20 }, IdxL { 2, 30 } }, { IdxL { 1, 10 }, IdxL { 2, 40 } }, { IdxL { 1, 50 } }, { IdxL { 2, 50 } } },
        //    { { IdxL { 0, 40 }, IdxL { 2, 10 } }, { IdxL { 0, 30 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 20 }, IdxL { 2, 30 } }, { IdxL { 0, 10 }, IdxL { 2, 40 } }, { IdxL { 0, 50 } }, { IdxL { 2, 50 } } },
        //    { { IdxL { 0, 40 }, IdxL { 1, 10 } }, { IdxL { 0, 30 }, IdxL { 1, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 0, 20 }, IdxL { 1, 30 } }, { IdxL { 0, 10 }, IdxL { 1, 40 } }, { IdxL { 0, 50 } }, { IdxL { 1, 50 } } },
        //    { { IdxL { 4, 30 } }, { IdxL { 0, 20 }, IdxL { 1, 20 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 30 } }, { IdxL { 1, 30 } }, { IdxL { 2, 30 } } },
        //    { { IdxL { 3, 30 } }, { IdxL { 0, 20 }, IdxL { 1, 20 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 30 } }, { IdxL { 1, 30 } }, { IdxL { 2, 30 } } },
        //};

        // sp = 0.9984416493816418, sq = 0.0015583506183533343
        vector<vector<vector<IdxL>>> table
        {
            { { IdxL { 1, 25 }, IdxL { 2, 25 } } },
            { { IdxL { 0, 25 }, IdxL { 2, 25 } } },
            { { IdxL { 0, 25 }, IdxL { 1, 25 } } },
            { { IdxL { 4, 30 } }, { IdxL { 0, 20 }, IdxL { 1, 20 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 30 } }, { IdxL { 1, 30 } }, { IdxL { 2, 30 } } },
            { { IdxL { 3, 30 } }, { IdxL { 0, 20 }, IdxL { 1, 20 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 30 } }, { IdxL { 1, 30 } }, { IdxL { 2, 30 } } }
        };

        // sp = 0.9984416493816418, sq = 0.0015583506183533343
        //vector<vector<vector<IdxL>>> table
        //{
        //    { { IdxL { 1, 40 }, IdxL { 2, 10 } }, { IdxL { 1, 30 }, IdxL { 2, 20 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 1, 20 }, IdxL { 2, 30 } }, { IdxL { 1, 10 }, IdxL { 2, 40 } } },
        //    { { IdxL { 0, 40 }, IdxL { 2, 10 } }, { IdxL { 0, 30 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 20 }, IdxL { 2, 30 } }, { IdxL { 0, 10 }, IdxL { 2, 40 } } },
        //    { { IdxL { 0, 40 }, IdxL { 1, 10 } }, { IdxL { 0, 30 }, IdxL { 1, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 0, 20 }, IdxL { 1, 30 } }, { IdxL { 0, 10 }, IdxL { 1, 40 } } },
        //    { { IdxL { 4, 30 } }, { IdxL { 0, 20 }, IdxL { 1, 20 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } } },
        //    { { IdxL { 3, 30 } }, { IdxL { 0, 20 }, IdxL { 1, 20 }, IdxL { 2, 20 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } } }
        //};

        array<string, all_count> element_names
        {
            "pr1" /* 0 */, "pr2" /* 1 */, "pr3" /* 2 */, "pr5" /* 3 */, "pr6" /* 4 */,
            "a1" /* 5 */, "a2" /* 6 */,
            "b1" /* 7 */, "b2" /* 8 */, "b4" /* 9 */, "b5" /* 10 */,
            "c1", /* 11 */ "c2" /* 12 */, "c4" /* 13 */, "c5" /* 14 */, "c6" /* 15 */,
            "d1", /* 16 */ "d2" /* 17 */, "d3" /* 18 */, "d6" /* 19 */, "d8" /* 20 */,
            "m1" /* 21 */, "m2" /* 22 */
        };

        const double qpr { 1.2e-4 };
        const double qpa { 1.2e-4 };
        const double qpb { 1.5e-5 };
        const double qpc { 4.1e-4 };
        const double qpd { 2.2e-5 };
        const double qpm { 3.6e-4 };

        array<double, all_count> q_values
        {
            qpr, qpr, qpr, qpr, qpr,
            qpa, qpa,
            qpb, qpb, qpb, qpb,
            qpc, qpc, qpc, qpc, qpc,
            qpd, qpd, qpd, qpd, qpd,
            qpm, qpm
        };

        array<double, all_count> p_values { };
        for (size_t i = 0; i < all_count; i++)
            p_values[i] = 1.0 - q_values[i];

        auto sfunc = [](const StateVector& sv)
        {
            span<bool> s = sv.all;

            bool f1 = (s[16] + s[17]) * s[11] * (s[17] + s[18]) * s[12] * (s[7] + s[8]);
            bool f2 = s[19] * s[13] * s[8];
            bool f3 = s[0] * s[1] * s[2];
            bool f4 = s[20] * (s[14] + s[15]) * s[9];
            bool f5 = s[3] * s[4] * s[10];
            bool f6 = s[5] * s[6] * (s[21] + s[22]);

            return f1 * f2 * f3 * f4 * f5 * f6;
        };
        Scheme scheme
        {
            .sfunc = sfunc,
            .p = Harray<double> { p_values },
            .q = Harray<double> { q_values },
            .element_names = Harray<string> { element_names },
            .rt = ReconfigurationTable
            {
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table,
                sfunc
            }
        };

        auto result
        {
            Utils::execution_time<SchemeReliability>(
                [&scheme]() { return calculate_reliability(scheme); }
            )
        };

        Utils::dump_text_summary(result);
        Utils::dump_binary_scored_state_set(result.scored_state_set, "v17_rt-scored-state-set.sstd");
    }
}
