export module sr_reconfiguration_table;

export import sr_state;
import sr_aliases;

import std;

using std::span;
using std::vector;
using std::stack;
using std::unordered_map;
using std::min_element;
using std::move;
using std::optional, std::nullopt;

export namespace sr
{
    struct IdxL
    {
        size_t index;
        double load;

        IdxL() = default;

        IdxL(size_t index, double load):
            index { index },
            load { load }
        { }
    };

    using Transition = vector<IdxL>;
    using TransitionSet = vector<Transition>;

    class ReconfigurationTable
    {
    private:

        const size_t processor_count;

        const Harray<double> normal_load;

        const Harray<double> max_load;

        const vector<TransitionSet> table;

        const SFunc sfunc;

    public:

        ReconfigurationTable(
            size_t processor_count,
            span<double> normal_load,
            span<double> max_load,
            vector<TransitionSet> table,
            SFunc sfunc
        );

        ReconfigurationTable(const ReconfigurationTable&) = default;
        ReconfigurationTable& operator=(const ReconfigurationTable&) = default;

        ReconfigurationTable(ReconfigurationTable&&) = default;
        ReconfigurationTable& operator=(ReconfigurationTable&&) = default;

        inline size_t get_processor_count() const noexcept
        {
            return processor_count;
        }

        StateVector reconfigure_state(const StateVector& sv1) const;

    private:

        optional<StateVector> traverse_reconfiguration_tree(
            const StateVector& sv1,
            stack<size_t>& failed_processor_indexes,
            unordered_map<size_t, Transition>& applied_transitions,
            vector<StateVector>& failed_state_vectors
        ) const;

        void apply_transition_to_load(
            const Transition& transition,
            Harray<double>& load
        ) const;

        bool is_transition_successful(
            const Transition& transition,
            const Harray<double>& reconfiguration_load
        ) const;
    };
}

module : private;

namespace sr
{
    ReconfigurationTable::ReconfigurationTable(
        size_t processor_count,
        span<double> normal_load,
        span<double> max_load,
        vector<TransitionSet> table,
        SFunc sfunc
    ):
        processor_count { processor_count },
        normal_load { normal_load },
        max_load { max_load },
        table { table },
        sfunc { sfunc }
    { }

    StateVector ReconfigurationTable::reconfigure_state(const StateVector& sv1) const
    {
        stack<size_t> failed_processors_indexes { };
        for (size_t i = 0; i < processor_count; i++)
            if (sv1.processors[i] == 0)
                failed_processors_indexes.push(i);
        if (failed_processors_indexes.empty())
            return sv1;
        
        vector<StateVector> failed_state_vectors { };
        unordered_map<size_t, Transition> applied_transitions { };
        optional<StateVector> reconfiguration_result
        {
            traverse_reconfiguration_tree(
                sv1,
                failed_processors_indexes,
                applied_transitions,
                failed_state_vectors
            )
        };

        return reconfiguration_result.value_or(
            failed_state_vectors.empty() ? sv1 : failed_state_vectors[0]
        );
    }

    optional<StateVector> ReconfigurationTable::traverse_reconfiguration_tree(
        const StateVector& sv1,
        stack<size_t>& failed_processor_indexes,
        unordered_map<size_t, Transition>& applied_transitions,
        vector<StateVector>& failed_state_vectors
    ) const {
        if (failed_processor_indexes.empty())
        {
            Harray<double> reconfiguration_load { normal_load };
            for (const auto& [pidx, transition] : applied_transitions)
                apply_transition_to_load(transition, reconfiguration_load);

            StateVector sv2 { sv1 };
            for (size_t i = 0; i < processor_count; i++)
            {
                if (sv1.processors[i] == 1 && reconfiguration_load[i] > max_load[i])
                {
                    sv2.processors[i] = 0;
                }
                else if (sv1.processors[i] == 0 &&
                         is_transition_successful(applied_transitions[i], reconfiguration_load))
                {
                    sv2.processors[i] = 1;
                }
            }

            if (sfunc(sv2))
            {
                return sv2;
            }
            else
            {
                failed_state_vectors.push_back(sv2);
                return nullopt;
            }
        }

        size_t current_processor_index { failed_processor_indexes.top() };
        for (const Transition& transition : table[current_processor_index])
        {
            failed_processor_indexes.pop();
            applied_transitions[current_processor_index] = transition;
            auto result =
                traverse_reconfiguration_tree(
                    sv1, failed_processor_indexes, applied_transitions, failed_state_vectors
                );

            if (result.has_value())
                return result;

            failed_processor_indexes.push(current_processor_index);
            applied_transitions.erase(current_processor_index);
        }
        return nullopt;
    }

    void ReconfigurationTable::apply_transition_to_load(
        const Transition& transition,
        Harray<double>& load
    ) const {
        for (const IdxL& increment : transition)
            load[increment.index] += increment.load;
    }

    bool ReconfigurationTable::is_transition_successful(
        const Transition& transition,
        const Harray<double>& reconfiguration_load
    ) const {
        if (transition.empty())
            return false;

        for (const auto& r : transition)
        {
            if (reconfiguration_load[r.index] > max_load[r.index])
                return false;
        }
        return true;
    }
}
