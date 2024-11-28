export module sr_state;

export import sr_harray;

import std;

using std::span;
using std::vector;
using std::pow;

export namespace sr
{
    struct StateVector
    {
        Harray<bool> sv;
        span<bool> all;
        span<bool> processors;

        StateVector() = default;

        StateVector(
            Harray<bool> sv,
            size_t processor_count
        ):
            sv { sv },
            all { this->sv.get_elements() },
            processors { all.subspan(0, processor_count) }
        { }

        StateVector(const StateVector& other): StateVector(other.sv, other.processors.size()) { }
        StateVector& operator=(const StateVector& other)
        {
            sv = other.sv;
            all = { sv.get_elements() };
            processors = { all.subspan(0, other.processors.size()) };

            return *this;
        }

        StateVector(StateVector&&) = default;
        StateVector& operator=(StateVector&&) = default;
    };

    class StateVectorGenerator
    {
    private:

        const size_t all_count;
        const size_t processor_count;
        const size_t full_2n_set_size;

    public:

        StateVectorGenerator(
            size_t all_count,
            size_t processor_count
        );

        vector<StateVector> generate_full_2n_state_vector_set() const;

    private:

        void traverse_state_tree(
            Harray<bool>& temp, size_t idx, vector<StateVector>& acc
        ) const;
    };
}

module : private;

namespace sr
{
    StateVectorGenerator::StateVectorGenerator(
        size_t all_count,
        size_t processor_count
    ):
        all_count { all_count },
        processor_count { processor_count },
        full_2n_set_size { static_cast<size_t>(pow(2, all_count)) }
    { }

    vector<StateVector> StateVectorGenerator::generate_full_2n_state_vector_set() const
    {
        vector<StateVector> result;
        result.reserve(full_2n_set_size);

        Harray<bool> temp { all_count };
        traverse_state_tree(temp, 0, result);

        return result;
    }

    void StateVectorGenerator::traverse_state_tree(
        Harray<bool>& temp, size_t idx, vector<StateVector>& acc
    ) const {
        if (idx == all_count)
        {
            acc.push_back(StateVector(temp, processor_count));
            return;
        }

        temp[idx] = 1;
        traverse_state_tree(temp, idx + 1, acc);

        temp[idx] = 0;
        traverse_state_tree(temp, idx + 1, acc);
    }
}
