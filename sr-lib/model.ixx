export module scheme_reliability:model;

import std;
using std::string;
using std::vector;
using std::array;
using std::span;
using std::function;
using std::filesystem::path;

namespace sr_impl::model
{
    struct IdxL
    {
        size_t index;
        double load;
    };

    using Transition = vector<IdxL>;
    using TransitionSet = vector<Transition>;

    struct Element
    {
        string name;
        double p;
        double q;
    };

    struct Processor
    {
        string name;
        double p;
        double q;
        double normal_load;
        double max_load;
        TransitionSet transitions;
    };

    template<size_t all_count, size_t processor_count>
    struct StateVector
    {
        static_assert(
            processor_count < all_count,
            "processor count must be less or equal than elements count"
        );

        array<bool, all_count> sv;
        span<bool> all;
        span<bool> processors;

        StateVector():
            sv { },
            all { sv.begin(), sv.end() },
            processors { all.subspan(0, processor_count) }
        { }

        StateVector(const StateVector& other):
            sv { other.sv },
            all { sv.begin(), sv.end() },
            processors { all.subspan(0, processor_count) }
        { }

        StateVector& operator=(const StateVector& other)
        {
            sv = other.sv;
            all = { sv.begin(), sv.end() };
            processors = { all.subspan(0, processor_count) };

            return *this;
        }
    };

    template <size_t all_count, size_t processor_count>
    using SchemeFunction = function<bool(const StateVector<all_count, processor_count>&)>;

    enum class SchemeType { Greedy, Brute };

    template<size_t all_count, size_t processor_count>
    struct Scheme
    {
        static_assert(
            processor_count < all_count,
            "processor count must be less or equal than elements count"
        );

        string scheme_name;

        array<Element, all_count - processor_count> elements;
        array<Processor, processor_count> processors;

        SchemeFunction<all_count, processor_count> scheme_function;
        SchemeType type;
    };

    template<size_t all_count, size_t processor_count>
    struct ScoredStateVector
    {
        static_assert(
            processor_count < all_count,
            "processor count must be less or equal than elements count"
        );

        bool scheme_state_sv1;
        bool scheme_state_sv2;
        bool scheme_state;
        double probability;
        array<bool, all_count> sv1;
        array<bool, all_count> sv2;

        size_t get_processor_count()
        {
            return processor_count;
        }
    };

    struct SchemeReliabilitySummary
    {
        double sp;
        double sq;
        size_t state_vector_set_count;
        path result_path;
    };
}
