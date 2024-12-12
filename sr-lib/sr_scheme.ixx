export module sr_scheme;

export import sr_aliases;
export import sr_reconfiguration_table;

import std;

using std::function;
using std::vector;
using std::span;
using std::thread;
using std::string;
using std::println;
using std::shared_ptr;
using std::move;

export namespace sr
{
    struct Scheme
    {
        SFunc sfunc;
        Harray<double> p;
        Harray<double> q;
        Harray<string> element_names;
        shared_ptr<ReconfigurationTable> rt;

        Scheme(
            SFunc sfunc,
            span<double> p,
            span<double> q,
            span<string> element_names,
            ReconfigurationTable* rt
        ):
            sfunc { sfunc },
            p { p }, q { q }, element_names { element_names },
            rt { rt }
        { }
            
        Scheme(const Scheme& other):
            sfunc { other.sfunc },
            p { other.p },
            q { other.q },
            element_names { other.element_names },
            rt { other.rt }
        { }

        Scheme(Scheme&& other):
            sfunc { move(other.sfunc) },
            p { move(other.p) },
            q { move(other.q) },
            element_names { move(other.element_names) },
            rt { move(other.rt) }
        { }

        inline size_t all_count() const noexcept
        {
            return p.size();
        }
        
        inline size_t processor_count() const noexcept
        {
            return rt->get_processor_count();
        }
    };

    struct ScoredStateVector
    {
        StateVector sv1;
        StateVector sv2;
        double probability;
        bool scheme_state_sv1;
        bool scheme_state_sv2;
        bool scheme_state;
    };

    struct SchemeReliability
    {
        Scheme scheme;
        vector<ScoredStateVector> scored_state_set;
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

        const unsigned int id;

        const Scheme& scheme;
        vector<StateVector> state_set;
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
            unsigned int id,
            const Scheme& scheme
        ):
            id { id },
            scheme { scheme },
            state_set { },
            sr
            {
                .scheme = scheme,
                .scored_state_set = { },
                .sp = 0, .sq = 0
            },
            t { }
        { }

        inline void assign_sv(StateVector& sv)
        {
            state_set.push_back(sv);
        }

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
                    for (const StateVector& sv1 : state_set)
                    {
                        StateVector sv2 = scheme.rt->reconfigure_state(sv1);

                        bool scheme_state_sv1 { scheme.sfunc(sv1) };
                        bool scheme_state_sv2 { scheme.sfunc(sv2) };
                        double probability { calculate_probability(scheme, sv1) };

                        if (scheme_state_sv2)
                            sr.sp += probability;
                        else
                            sr.sq += probability;

                        sr.scored_state_set.push_back(
                            {
                                .sv1 = sv1,
                                .sv2 = sv2,
                                .probability = probability,
                                .scheme_state_sv1 = scheme_state_sv1,
                                .scheme_state_sv2 = scheme_state_sv2,
                                .scheme_state = scheme_state_sv2
                            }
                        );
                    }
                    println("worker {} done", id);
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

            println("full state set size = {}", full_state_set.size());

            vector<StateSetReliabilityCalculator> workers
            {
                split_set_into_workers(
                    scheme, span { full_state_set.data(), full_state_set.size() }
                )
            };

            println("worker count = {}", workers.size());

            for (StateSetReliabilityCalculator& w : workers)
                w.execute(full_state_set.size());

            for (StateSetReliabilityCalculator& w : workers)
                w.join();


            return join_workers(
                scheme, workers, full_state_set.size(), scheme.all_count()
            );
        }

    private:

        static vector<StateSetReliabilityCalculator> split_set_into_workers(
            const Scheme& scheme,
            span<StateVector> state_set
        ) {
            auto thread_count = thread::hardware_concurrency();

            vector<StateSetReliabilityCalculator> result { };
            result.reserve(thread_count);

            for (unsigned int i = 0; i < thread_count; i++)
                result.emplace_back(i, scheme);

            for (size_t i = 0; i < state_set.size(); i++)
                result[i % thread_count].assign_sv(state_set[i]);

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
                .sp = 0, .sq = 0
            };
            result.scored_state_set.reserve(full_state_set_size);

            for (const StateSetReliabilityCalculator& w : workers)
            {
                result.scored_state_set.append_range(w.get_scheme_reliability().scored_state_set);

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
