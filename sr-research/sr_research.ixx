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
using std::type_info;

export namespace sr::research
{
    void simple();
    void s23_original();
    void s23_rt_7_7_7_8_8();
}

module : private;

namespace sr::research
{
    class Utils
    {
    private:

        static void dump_binary_scored_state_set_data(
            const vector<ScoredStateVector>& scored_state_set,
            const string& data_filename
        ) {
            ofstream data_file { data_filename, std::ios::binary };
            if (!data_file.is_open())
            {
                println(cerr, "Error: can't open data_file {} for dumping scored state set", data_filename);
                return;
            }

            const size_t bufsize { 8768 };
            unique_ptr<char[]> buffer { new char[bufsize] };
            data_file.rdbuf()->pubsetbuf(buffer.get(), bufsize);

            size_t tmp { 23 };
            for (const ScoredStateVector& ssv : scored_state_set)
            {
                if (ssv.sv1.all.size() != tmp)
                {
                    println("wtf1");
                }
                if (ssv.sv2.all.size() != tmp)
                {
                    println("wtf2 | {}", ssv.sv2.all.size());
                }
                data_file.write(reinterpret_cast<const char*>(&ssv.scheme_state_sv1), sizeof(bool));
                data_file.write(reinterpret_cast<const char*>(&ssv.scheme_state_sv2), sizeof(bool));
                data_file.write(reinterpret_cast<const char*>(&ssv.scheme_state), sizeof(bool));
                data_file.write(reinterpret_cast<const char*>(&ssv.probability), sizeof(double));
                data_file.write(reinterpret_cast<const char*>(ssv.sv1.sv.get_data()), ssv.sv1.all.size() * sizeof(bool));
                data_file.write(reinterpret_cast<const char*>(ssv.sv2.sv.get_data()), ssv.sv2.all.size() * sizeof(bool));
            }

            data_file.flush();
        }

        static void dump_scheme_reliability_elements_info(
            const span<string> element_names,
            const string& elements_filename
        ) {
            ofstream elements_file { elements_filename, std::ios::trunc };
            if (!elements_file.is_open())
            {
                println(cerr, "Error: can't open elements_file {} for dumping elements info", elements_filename);
                return;
            }

            elements_file << element_names.front();

            for (auto it = element_names.begin() + 1; it != element_names.end(); ++it)
            {
                elements_file << "," << *it;
            }

            elements_file.flush();
        }

    public:

        static const string BINARY_SCORED_STATE_SET_DATA_EXTENSION;
        static const string SCHEME_RELIABILITY_ELEMENTS_EXTENSION;

        Utils() = delete;

        static void dump_text_summary(const SchemeReliability& sr)
        {
            println("sp = {}, sq = {}", sr.sp, sr.sq);
            println("state count = {}", sr.scored_state_set.size());
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

        static ReconfigurationTable* create_log_brute_force(
            size_t processor_count,
            span<double> normal_load,
            span<double> max_load,
            vector<TransitionSet>& table,
            SFunc sfunc
        ) {
            ReconfigurationTable* result
            {
                new BruteForceReconfigurationTable
                {
                    processor_count,
                    span<double> { normal_load },
                    span<double> { max_load },
                    table,
                    sfunc
                }
            };
            println("rt type is {}", typeid(*result).name());
            return result;
        }

        static ReconfigurationTable* create_log_greedy(
            size_t processor_count,
            span<double> normal_load,
            span<double> max_load,
            vector<TransitionSet> table
        ) {
            ReconfigurationTable* result
            {
                new GreedyReconfigurationTable
                {
                    processor_count,
                    span<double> { normal_load },
                    span<double> { max_load },
                    table
                }
            };
            println("rt type is {}", typeid(*result).name());
            return result;
        }

        static void process_scheme(
            const Scheme& scheme,
            const string& output_name
        ) {
            auto result
            {
                Utils::execution_time<SchemeReliability>(
                    [&scheme]() { return calculate_reliability(scheme); }
                )
            };

            Utils::dump_text_summary(result);

            println("start dumping scheme reliability");

            Utils::dump_binary_scored_state_set_data(
                result.scored_state_set,
                format("{}-data.{}", output_name, Utils::BINARY_SCORED_STATE_SET_DATA_EXTENSION)
            );

            Utils::dump_scheme_reliability_elements_info(
                result.scheme.element_names.get_elements(),
                format("{}-element-names.{}", output_name, Utils::SCHEME_RELIABILITY_ELEMENTS_EXTENSION)
            );

            println("saved scheme reliability");
        }
    };

    const string Utils::BINARY_SCORED_STATE_SET_DATA_EXTENSION { "ssv" };
    const string Utils::SCHEME_RELIABILITY_ELEMENTS_EXTENSION { "ens" };

    void simple()
    {
        const size_t all_count { 8 };
        const size_t processor_count { 4 };

        array<double, processor_count> normal_load_values { 40, 20, 30, 30 };
        array<double, processor_count> max_load_values { 100, 100, 50, 50 };

        vector<TransitionSet> table
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
            sfunc,
            span<double> { p_values },
            span<double> { q_values },
            span<string> { element_names },
            Utils::create_log_brute_force(
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table,
                sfunc
            )
        };

        Utils::process_scheme(scheme, "simple");
    }

    void s23_original()
    {
        const size_t all_count { 23 };
        const size_t processor_count { 5 };

        array<double, processor_count> normal_load_values { 50, 50, 50, 30, 30 };
        array<double, processor_count> max_load_values { 80, 80, 80, 60, 60 };

        vector<TransitionSet> table
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

        Scheme bscheme
        {
            sfunc,
            span<double> { p_values },
            span<double> { q_values },
            span<string> { element_names },
            Utils::create_log_brute_force(
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table,
                sfunc
            )
        };
        Utils::process_scheme(bscheme, "s23-original-brute");
        
        Scheme gscheme
        {
            sfunc,
            span<double> { p_values },
            span<double> { q_values },
            span<string> { element_names },
            Utils::create_log_greedy(
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table
            )
        };
        Utils::process_scheme(gscheme, "s23-original-greedy");
    }

    void s23_rt_7_7_7_8_8()
    {
        const size_t all_count { 23 };
        const size_t processor_count { 5 };

        array<double, processor_count> normal_load_values { 50, 50, 50, 30, 30 };
        array<double, processor_count> max_load_values { 100, 100, 100, 60, 60 };

        vector<TransitionSet> table
        {
            { { IdxL { 1, 50 } }, { IdxL { 2, 50 } }, { IdxL { 1, 25 }, IdxL { 2, 25 } }, { IdxL { 1, 25 }, IdxL { 3, 30 } }, { IdxL { 1, 25 }, IdxL { 4, 30 } }, { IdxL { 2, 25 }, IdxL { 3, 30 } }, { IdxL { 2, 25 }, IdxL { 4, 30 } } },
            { { IdxL { 0, 50 } }, { IdxL { 2, 50 } }, { IdxL { 0, 25 }, IdxL { 2, 25 } }, { IdxL { 0, 25 }, IdxL { 3, 30 } }, { IdxL { 0, 25 }, IdxL { 4, 30 } }, { IdxL { 2, 25 }, IdxL { 3, 30 } }, { IdxL { 2, 25 }, IdxL { 4, 30 } } },
            { { IdxL { 0, 50 } }, { IdxL { 1, 50 } }, { IdxL { 0, 25 }, IdxL { 1, 25 } }, { IdxL { 0, 25 }, IdxL { 3, 30 } }, { IdxL { 0, 25 }, IdxL { 4, 30 } }, { IdxL { 1, 25 }, IdxL { 3, 30 } }, { IdxL { 1, 25 }, IdxL { 4, 30 } } },
            { { IdxL { 4, 30 } }, { IdxL { 0, 35 } }, { IdxL { 1, 35 } }, { IdxL { 2, 35 } }, { IdxL { 0, 18 }, IdxL { 1, 18 } }, { IdxL { 1, 18 }, IdxL { 2, 18 } }, { IdxL { 0, 18 }, IdxL { 2, 18 } }, { IdxL { 0, 12 }, IdxL { 1, 12 }, IdxL { 2, 12 } } },
            { { IdxL { 3, 30 } }, { IdxL { 0, 35 } }, { IdxL { 1, 35 } }, { IdxL { 2, 35 } }, { IdxL { 0, 18 }, IdxL { 1, 18 } }, { IdxL { 1, 18 }, IdxL { 2, 18 } }, { IdxL { 0, 18 }, IdxL { 2, 18 } }, { IdxL { 0, 12 }, IdxL { 1, 12 }, IdxL { 2, 12 } } },
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

        Scheme gscheme
        {
            sfunc,
            span<double> { p_values },
            span<double> { q_values },
            span<string> { element_names },
            Utils::create_log_greedy(
                processor_count,
                span<double> { normal_load_values },
                span<double> { max_load_values },
                table
            )
        };
        Utils::process_scheme(gscheme, "s23-77788-greedy");

        //Scheme bscheme
        //{
        //    sfunc,
        //    span<double> { p_values },
        //    span<double> { q_values },
        //    span<string> { element_names },
        //    Utils::create_log_brute_force(
        //        processor_count,
        //        span<double> { normal_load_values },
        //        span<double> { max_load_values },
        //        table,
        //        sfunc
        //    )
        //};
        //Utils::process_scheme(bscheme, "s23-77788-brute");
    }
}
