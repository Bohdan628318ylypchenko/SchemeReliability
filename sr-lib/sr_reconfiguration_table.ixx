export module sr_reconfiguration_table;

export import sr_state;
import sr_aliases;

import std;

using std::span;
using std::vector;
using std::stack;
using std::unordered_map;
using std::move;
using std::count;

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

        struct ReconfigurationTreeTraversionResult
        {
            StateVector best;
            StateVector best_zero;
            size_t best_zero_active_count;
        };

        bool traverse_reconfiguration_tree(
            const StateVector& sv1,
            stack<size_t>& failed_processor_indexes,
            unordered_map<size_t, const Transition*>& applied_transitions,
            span<double>& reconfiguration_load,
            ReconfigurationTreeTraversionResult& result
        ) const;

        void apply_transition_to_load(
            const Transition& transition,
            span<double>& load,
            double sign
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
        
        unordered_map<size_t, const Transition*> applied_transitions { };
        Harray<double> reconfiguration_load_memory { normal_load  };
        span<double> reconfiguration_load { reconfiguration_load_memory.get_elements() };
        ReconfigurationTreeTraversionResult result
        {
            .best = { },
            .best_zero = { },
            .best_zero_active_count = 0
        };
        bool is_success
        {
            traverse_reconfiguration_tree(
                sv1,
                failed_processors_indexes,
                applied_transitions,
                reconfiguration_load,
                result
            )
        };

        return is_success ? result.best : result.best_zero;
    }

    bool ReconfigurationTable::traverse_reconfiguration_tree(
        const StateVector& sv1,
        stack<size_t>& failed_processor_indexes,
        unordered_map<size_t, const Transition*>& applied_transitions,
        span<double>& reconfiguration_load,
        ReconfigurationTreeTraversionResult& result
    ) const {
        if (failed_processor_indexes.empty())
        {
            StateVector sv2 { sv1 };
            for (size_t i = 0; i < processor_count; i++)
            {
                if (sv1.processors[i] == 1 && reconfiguration_load[i] > max_load[i])
                {
                    sv2.processors[i] = 0;
                }
                else if (sv1.processors[i] == 0 &&
                         is_transition_successful(*applied_transitions[i], reconfiguration_load))
                {
                    sv2.processors[i] = 1;
                }
            }

            if (sfunc(sv2))
            {
                result.best = move(sv2);
                return true;
            }
            else
            {
                size_t current_active_count { static_cast<size_t>(count(sv2.processors.begin(), sv2.processors.end(), true)) };
                if (current_active_count > result.best_zero_active_count)
                {
                    result.best_zero = move(sv2);
                    result.best_zero_active_count = current_active_count;
                }
                return false;
            }
        }

        size_t current_processor_index { failed_processor_indexes.top() };
        failed_processor_indexes.pop();
        for (const Transition& transition : table[current_processor_index])
        {
            applied_transitions[current_processor_index] = &transition;
            apply_transition_to_load(transition, reconfiguration_load, 1.0);
            bool is_success
            {
                traverse_reconfiguration_tree(
                    sv1, failed_processor_indexes, applied_transitions, reconfiguration_load, result
                )
            };

            if (is_success) return true;

            apply_transition_to_load(transition, reconfiguration_load, -1.0);
            applied_transitions.erase(current_processor_index);
        }
        failed_processor_indexes.push(current_processor_index);
        return false;
    }

    void ReconfigurationTable::apply_transition_to_load(
        const Transition& transition,
        span<double>& load,
        double sign
    ) const {
        for (const IdxL& increment : transition)
            load[increment.index] += sign * increment.load;
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
