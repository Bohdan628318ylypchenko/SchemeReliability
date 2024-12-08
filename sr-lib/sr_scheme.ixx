export module sr_scheme;

export import sr_reconfiguration_table;

import std;

using std::function;
using std::vector;
using std::fill;
using std::span;
using std::thread;
using std::string;
using std::print;

export namespace sr
{
    using SFunc = function<bool(const StateVector&)>;

    struct Scheme
    {
        SFunc sfunc;
        Harray<double> p;
        Harray<double> q;
        Harray<string> element_names;
        ReconfigurationTable rt;

        inline size_t all_count() const noexcept
        {
            return p.size();
        }
        
        inline size_t processor_count() const noexcept
        {
            return rt.get_processor_count();
        }
    };

    struct ScoredStateVector
    {
        StateVector sv1;
        StateVector sv2;
        double probability;
        bool scheme_state;
    };

    struct SchemeReliability
    {
        Scheme scheme;
        vector<ScoredStateVector> scored_state_set;
        Harray<size_t> fail_count_per_element_sv2;
        Harray<double> fail_probability_per_element_sv2;
        double sp;
        double sq;
    };

    SchemeReliability calculate_reliability(const Scheme& scheme);
}

module : private;

namespace sr
{
    class StateSetReliabilityCalculator
    {
    private:

        const Scheme& scheme;
        const span<StateVector> state_set;
        SchemeReliability sr;

        thread t;

        static double calculate_probability(const Scheme& scheme, const StateVector& sv)
        {
            double result { 1 };
            for (size_t i = 0; i < sv.all.size(); i++)
                result *= (sv.all[i] * scheme.p[i] + (1 - sv.all[i]) * scheme.q[i]);
            return result;
        }

    public:

        StateSetReliabilityCalculator() = default;

        StateSetReliabilityCalculator(
            const Scheme& scheme,
            const span<StateVector> state_set
        ):
            scheme { scheme },
            state_set { state_set },
            sr
            {
                .scheme = scheme,
                .scored_state_set = { },
                .fail_count_per_element_sv2 = { scheme.all_count() },
                .fail_probability_per_element_sv2 = { scheme.all_count() },
                .sp = 0, .sq = 0
            },
            t { }
        { }

        inline const SchemeReliability& get_scheme_reliability() const noexcept
        {
            return sr;
        }

        void execute(size_t full_state_set_size)
        {
            t = thread
            {
                [this, full_state_set_size]()
                {
                    fill(
                        sr.fail_count_per_element_sv2.get_elements().begin(),
                        sr.fail_count_per_element_sv2.get_elements().end(),
                        0
                    );
                    fill(
                        sr.fail_probability_per_element_sv2.get_elements().begin(),
                        sr.fail_probability_per_element_sv2.get_elements().end(),
                        0
                    );

                    for (const StateVector& sv1 : state_set)
                    {
                        StateVector sv2 { sv1 };
                        scheme.rt.reconfigure_state(sv1, sv2);

                        bool scheme_state { scheme.sfunc(sv2) };
                        double probability { calculate_probability(scheme, sv1) };

                        if (scheme_state)
                            sr.sp += probability;
                        else
                            sr.sq += probability;

                        if (!scheme_state)
                        {
                            for (size_t i = 0; i < scheme.all_count(); i++)
                            {
                                if (!sv2.all[i])
                                {
                                    sr.fail_count_per_element_sv2[i]++;
                                    sr.fail_probability_per_element_sv2[i] += probability;
                                }
                            }
                        }

                        sr.scored_state_set.push_back(
                            {
                                .sv1 = sv1,
                                .sv2 = sv2,
                                .probability = probability,
                                .scheme_state = scheme_state
                            }
                        );
                    }
                }
            };
        }

        void join()
        {
            t.join();
        }
    };

    class SchemeReliabilityCalculator
    {
    public:

        SchemeReliabilityCalculator() = delete;

        static SchemeReliability calculate_reliability(const Scheme& scheme)
        {
            vector<StateVector> full_state_set
            {
                StateVectorGenerator { scheme.all_count(), scheme.processor_count() }.generate_full_2n_state_vector_set()
            };

            print("full state set size = {}\n", full_state_set.size());

            vector<StateSetReliabilityCalculator> workers
            {
                split_set_into_workers(
                    scheme, span { full_state_set.data(), full_state_set.size() }
                )
            };

            print("worker count = {}\n", workers.size());

            for (StateSetReliabilityCalculator& w : workers)
                w.execute(full_state_set.size());

            for (StateSetReliabilityCalculator& w : workers)
                w.join();

            return join_workers(scheme, workers, full_state_set.size(), scheme.all_count());
        }

    private:

        static vector<StateSetReliabilityCalculator> split_set_into_workers(
            const Scheme& scheme,
            span<StateVector> state_set
        ) {
            auto thread_count = thread::hardware_concurrency();

            auto worker_state_set_size = state_set.size() / thread_count;
            auto leftover = state_set.size() % thread_count;

            vector<StateSetReliabilityCalculator> result { };
            result.reserve(thread_count);

            for (unsigned int i = 0; i < thread_count; i++)
            {
                result.push_back(
                    {
                        scheme,
                        state_set.subspan(
                            i * worker_state_set_size,
                            worker_state_set_size
                        )
                    }
                );
            }
            if (leftover != 0)
            {
                result.push_back(
                    {
                        scheme,
                        state_set.subspan(
                            thread_count * worker_state_set_size,
                            leftover
                        )
                    }
                );
            }

            return result;
        }

        static SchemeReliability join_workers(
            const Scheme& scheme,
            const vector<StateSetReliabilityCalculator>& workers,
            size_t full_state_set_size,
            size_t all_count
        ) {
            SchemeReliability result
            {
                .scheme = scheme,
                .scored_state_set = vector<ScoredStateVector> { },
                .fail_count_per_element_sv2 = Harray<size_t>(all_count),
                .fail_probability_per_element_sv2 = Harray<double>(all_count),
                .sp = 0, .sq = 0
            };
            result.scored_state_set.reserve(full_state_set_size);

            fill(
                result.fail_count_per_element_sv2.get_elements().begin(),
                result.fail_count_per_element_sv2.get_elements().end(),
                0
            );

            fill(
                result.fail_probability_per_element_sv2.get_elements().begin(),
                result.fail_probability_per_element_sv2.get_elements().end(),
                0
            );

            for (const StateSetReliabilityCalculator& w : workers)
            {
                result.scored_state_set.append_range(w.get_scheme_reliability().scored_state_set);

                for (size_t i = 0; i < all_count; i++)
                {
                    result.fail_count_per_element_sv2[i]+=
                        w.get_scheme_reliability().fail_count_per_element_sv2[i];
                    result.fail_probability_per_element_sv2[i] +=
                        w.get_scheme_reliability().fail_probability_per_element_sv2[i];
                }

                result.sp += w.get_scheme_reliability().sp;
                result.sq += w.get_scheme_reliability().sq;
            }

            return result;
        }
    };

    SchemeReliability calculate_reliability(const Scheme& scheme)
    {
        return SchemeReliabilityCalculator::calculate_reliability(scheme);
    }
}
