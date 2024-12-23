export module research;

import scheme_reliability;
using namespace sr;

import std;

using std::function;
using std::string;
using std::span;
using std::format;
using std::println, std::print;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::seconds;

export namespace research
{
    void simple();
    void s23_original();
    void s23_rt_7_7_7_8_8();
    void s23_rt_7_7_7_8_8_modified_connections();
    void s24_d9_right();
    void s25_d9_d10_right();
    void s27_d9_d10_c7_right_c8_left();
    void s29_d9_d10_c7_right_c8_left_a4();
    void s26_final();
}

module : private;

namespace research
{
    class Utils
    {
    public:

        static const string BINARY_SCORED_STATE_SET_DATA_EXTENSION;
        static const string SCHEME_RELIABILITY_ELEMENTS_EXTENSION;

        Utils() = delete;

        static void dump_text_summary(const SchemeReliabilitySummaryDto& sr)
        {
            println("path = {}", sr.result_path.string());
            println("sp = {}, sq = {}", sr.sp, sr.sq);
            println("state count = {}", sr.state_vector_set_count);
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

        template<size_t all_count, size_t processor_count>
        static void process_scheme(const SchemeDto<all_count, processor_count>& scheme)
        {
            print("\nScheme type = {}\n", scheme.type == SchemeType::Brute ? "brute" : "greedy");
            auto result
            {
                Utils::execution_time<SchemeReliabilitySummaryDto>(
                    [&scheme]() { return calculate_scheme_reliability<all_count, processor_count>(scheme); }
                )
            };
            Utils::dump_text_summary(result);
        }
    };

    const string Utils::BINARY_SCORED_STATE_SET_DATA_EXTENSION { "ssv" };
    const string Utils::SCHEME_RELIABILITY_ELEMENTS_EXTENSION { "ens" };

    constexpr double qpr { 1.2e-4 };
    constexpr double qpa { 1.2e-4 };
    constexpr double qpb { 1.5e-5 };
    constexpr double qpc { 4.1e-4 };
    constexpr double qpd { 2.2e-5 };
    constexpr double qpm { 3.6e-4 };

    constexpr double ppr { 1.0 - qpr };
    constexpr double ppa { 1.0 - qpa };
    constexpr double ppb { 1.0 - qpb };
    constexpr double ppc { 1.0 - qpc };
    constexpr double ppd { 1.0 - qpd };
    constexpr double ppm { 1.0 - qpm };

    void simple()
    {
        constexpr size_t all_count { 8 };
        constexpr size_t processor_count { 4 };

        SchemeDto<all_count, processor_count> scheme
        {
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
            }
        };

        scheme.scheme_name = "simple-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "simple-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s23_original()
    {
        constexpr size_t all_count { 23 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 80,
                    .transitions = { { TrUnit { 1, 25 }, TrUnit { 2, 25 } } }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 80,
                    .transitions = { { TrUnit { 0, 25 }, TrUnit { 2, 25 } } }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 80,
                    .transitions = { { TrUnit { 0, 25 }, TrUnit { 1, 25 } } }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions = { }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions = { }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = ((s[16] + s[17]) * s[11] + (s[17] + s[18]) * s[12]) * (s[7] + s[8]);
                bool f2 = s[19] * s[13] * s[8];
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = s[20] * (s[14] + s[15]) * s[9];
                bool f5 = s[3] * s[4] * s[10];
                bool f6 = s[5] * s[6] * (s[21] + s[22]);

                return f1 * f2 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s23-original-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s23-original-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s23_rt_7_7_7_8_8()
    {
        constexpr size_t all_count { 23 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 1, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 1, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 1, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 1, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 4, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 3, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = ((s[16] + s[17]) * s[11] + (s[17] + s[18]) * s[12]) * (s[7] + s[8]);
                bool f2 = s[19] * s[13] * s[8];
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = s[20] * (s[14] + s[15]) * s[9];
                bool f5 = s[3] * s[4] * s[10];
                bool f6 = s[5] * s[6] * (s[21] + s[22]);

                return f1 * f2 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s23-77788-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s23-77788-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s23_rt_7_7_7_8_8_modified_connections()
    {
        constexpr size_t all_count { 23 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 1, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 1, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 1, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 1, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 4, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 3, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = (s[16] + s[17] + s[18] + s[19]) * (s[11] + s[12] + s[13]) * (s[7] + s[8]);
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = s[20] * (s[14] + s[15]) * (s[9] + s[10]);
                bool f5 = s[3] * s[4];
                bool f6 = s[5] * s[6] * (s[21] + s[22]);

                return f1 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s23-77788-modified-connections-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s23-77788-modified-connections-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s24_d9_right()
    {
        constexpr size_t all_count { 24 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm },
                ElementDto { .name = "d9", .p = ppd, .q = qpd }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 1, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 1, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 1, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 1, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 4, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 3, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = (s[16] + s[17] + s[18] + s[19]) * (s[11] + s[12] + s[13]) * (s[7] + s[8]);
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = (s[20] + s[23]) * (s[14] + s[15]) * (s[9] + s[10]);
                bool f5 = s[3] * s[4];
                bool f6 = s[5] * s[6] * (s[21] + s[22]);

                return f1 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s24-d9-right-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s24-d9-right-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s25_d9_d10_right()
    {
        constexpr size_t all_count { 25 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm },
                ElementDto { .name = "d9", .p = ppd, .q = qpd },
                ElementDto { .name = "d10", .p = ppd, .q = qpd }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 1, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 1, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 1, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 1, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 4, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 3, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = (s[16] + s[17] + s[18] + s[19]) * (s[11] + s[12] + s[13]) * (s[7] + s[8]);
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = (s[20] + s[23] + s[24]) * (s[14] + s[15]) * (s[9] + s[10]);
                bool f5 = s[3] * s[4];
                bool f6 = s[5] * s[6] * (s[21] + s[22]);

                return f1 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s25-d9-d10-right-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s25-d9-d10-right-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s27_d9_d10_c7_right_c8_left()
    {
        constexpr size_t all_count { 27 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm },
                ElementDto { .name = "d9", .p = ppd, .q = qpd },
                ElementDto { .name = "d10", .p = ppd, .q = qpd },
                ElementDto { .name = "c7", .p = ppc, .q = qpc },
                ElementDto { .name = "c8", .p = ppc, .q = qpc }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 1, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 1, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 1, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 1, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 4, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 3, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = (s[16] + s[17] + s[18] + s[19]) * (s[11] + s[12] + s[13] + s[26]) * (s[7] + s[8]);
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = (s[20] + s[23] + s[24]) * (s[14] + s[15] + s[25]) * (s[9] + s[10]);
                bool f5 = s[3] * s[4];
                bool f6 = s[5] * s[6] * (s[21] + s[22]);

                return f1 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s27-d9-d10-c7-right-c8-left-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s27-d9-d10-c7-right-c8-left-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s29_d9_d10_c7_right_c8_left_a4()
    {
        constexpr size_t all_count { 29 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm },
                ElementDto { .name = "d9", .p = ppd, .q = qpd },
                ElementDto { .name = "d10", .p = ppd, .q = qpd },
                ElementDto { .name = "c7", .p = ppc, .q = qpc },
                ElementDto { .name = "c8", .p = ppc, .q = qpc },
                ElementDto { .name = "a3", .p = ppa, .q = qpa },
                ElementDto { .name = "a4", .p = ppa, .q = qpa }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 1, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 1, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 1, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 1, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 4, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 3, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = (s[16] + s[17] + s[18] + s[19]) * (s[11] + s[12] + s[13] + s[26]) * (s[7] + s[8]);
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = (s[20] + s[23] + s[24]) * (s[14] + s[15] + s[25]) * (s[9] + s[10]);
                bool f5 = s[3] * s[4];
                bool f6 = (s[5] + s[27]) * (s[6] + s[28]) * (s[21] + s[22]);

                return f1 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s29-d9-d10-c7-right-c8-left-a4-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s29-d9-d10-c7-right-c8-left-a4-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }

    void s26_final()
    {
        constexpr size_t all_count { 26 };
        constexpr size_t processor_count { 5 };

        SchemeDto<all_count, processor_count> scheme
        {
            .elements =
            {
                ElementDto { .name = "a1", .p = ppa, .q = qpa },
                ElementDto { .name = "a2", .p = ppa, .q = qpa },
                ElementDto { .name = "b1", .p = ppb, .q = qpb },
                ElementDto { .name = "b2", .p = ppb, .q = qpb },
                ElementDto { .name = "b4", .p = ppb, .q = qpb },
                ElementDto { .name = "b5", .p = ppb, .q = qpb },
                ElementDto { .name = "c1", .p = ppc, .q = qpc },
                ElementDto { .name = "c2", .p = ppc, .q = qpc },
                ElementDto { .name = "c4", .p = ppc, .q = qpc },
                ElementDto { .name = "c5", .p = ppc, .q = qpc },
                ElementDto { .name = "c6", .p = ppc, .q = qpc },
                ElementDto { .name = "d1", .p = ppd, .q = qpd },
                ElementDto { .name = "d2", .p = ppd, .q = qpd },
                ElementDto { .name = "d3", .p = ppd, .q = qpd },
                ElementDto { .name = "d6", .p = ppd, .q = qpd },
                ElementDto { .name = "d8", .p = ppd, .q = qpd },
                ElementDto { .name = "m1", .p = ppm, .q = qpm },
                ElementDto { .name = "m2", .p = ppm, .q = qpm },
                ElementDto { .name = "d9", .p = ppd, .q = qpd },
                ElementDto { .name = "a3", .p = ppa, .q = qpa },
                ElementDto { .name = "a4", .p = ppa, .q = qpa }
            },
            .processors =
            {
                ProcessorDto
                {
                    .name = "pr1", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 1, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 1, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr2", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 2, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 2, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 2, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr3", .p = ppr, .q = qpr, .normal_load = 50, .max_load = 100,
                    .transitions =
                    {
                        { TrUnit { 0, 50 } },
                        { TrUnit { 1, 50 } },
                        { TrUnit { 0, 25 }, TrUnit { 1, 25 } },
                        { TrUnit { 0, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 0, 25 }, TrUnit { 4, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 3, 30 } },
                        { TrUnit { 1, 25 }, TrUnit { 4, 30 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr4", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 4, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                },
                ProcessorDto
                {
                    .name = "pr5", .p = ppr, .q = qpr, .normal_load = 30, .max_load = 60,
                    .transitions =
                    {
                        { TrUnit { 3, 30 } },
                        { TrUnit { 0, 35 } },
                        { TrUnit { 1, 35 } },
                        { TrUnit { 2, 35 } },
                        { TrUnit { 0, 18 }, TrUnit { 1, 18 } },
                        { TrUnit { 1, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 18 }, TrUnit { 2, 18 } },
                        { TrUnit { 0, 12 }, TrUnit { 1, 12 }, TrUnit { 2, 12 } }
                    }
                }
            },
            .scheme_function = [](const StateVectorDto<all_count, processor_count>& sv)
            {
                span<bool> s = sv.all;

                bool f1 = (s[16] + s[17] + s[18] + s[19]) * (s[11] + s[12] + s[13]) * (s[7] + s[8]);
                bool f3 = s[0] * s[1] * s[2];
                bool f4 = (s[20] + s[23]) * (s[14] + s[15]) * (s[9] + s[10]);
                bool f5 = s[3] * s[4];
                bool f6 = (s[5] + s[24]) * (s[6] + s[25]) * (s[21] + s[22]);

                return f1 * f3 * f4 * f5 * f6;
            }
        };

        scheme.scheme_name = "s26-final-greedy";
        scheme.type = SchemeType::Greedy;
        Utils::process_scheme(scheme);

        scheme.scheme_name = "s26-final-brute";
        scheme.type = SchemeType::Brute;
        Utils::process_scheme(scheme);
    }
}
