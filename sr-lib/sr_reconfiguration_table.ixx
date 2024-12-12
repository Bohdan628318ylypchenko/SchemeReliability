export module sr_reconfiguration_table;

export import sr_state;
export import sr_aliases;

import std;

using std::span;
using std::vector;
using std::stack;
using std::unordered_map;
using std::move;
using std::count;
using std::min_element;
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
    protected:

        const size_t processor_count;

        const Harray<double> normal_load;

        const Harray<double> max_load;

        const vector<TransitionSet> table;

        ReconfigurationTable(
            size_t processor_count,
            span<double> normal_load,
            span<double> max_load,
            vector<TransitionSet> table
        ):
            processor_count { processor_count },
            normal_load { normal_load },
            max_load { max_load },
            table { table }
        { }

    public:

        ReconfigurationTable() = delete;
        virtual ~ReconfigurationTable() = default;

        ReconfigurationTable(const ReconfigurationTable&) = default;
        ReconfigurationTable& operator=(const ReconfigurationTable&) = default;

        ReconfigurationTable(ReconfigurationTable&&) = default;
        ReconfigurationTable& operator=(ReconfigurationTable&&) = default;

        inline size_t get_processor_count() const noexcept
        {
            return processor_count;
        }

        virtual StateVector reconfigure_state(const StateVector& sv1) const = 0;

    protected:

        void apply_transition_to_load(
            const Transition& transition,
            span<double> load,
            double sign
        ) const;

        bool is_transition_valid(
            const StateVector& sv, const Transition& transition
        ) const;

        bool is_transition_successful(
            const StateVector& original_sv,
            const Transition& transition,
            const Harray<double>& reconfiguration_load
        ) const;
    };

    class BruteForceReconfigurationTable : public ReconfigurationTable
    {
    private:

        const SFunc sfunc;

    public:

        BruteForceReconfigurationTable(
            size_t processor_count,
            span<double> normal_load,
            span<double> max_load,
            vector<TransitionSet> table,
            SFunc sfunc
        ):
            ReconfigurationTable(
                processor_count,
                normal_load,
                max_load,
                table
            ),
            sfunc { sfunc }
        { }

        StateVector reconfigure_state(const StateVector& sv1) const override;

    private:

        struct ReconfigurationTreeTraversionResult
        {
            StateVector best;
            StateVector best_zero;
            long long best_zero_active_count;
        };

        bool traverse_reconfiguration_tree(
            const StateVector& sv1,
            stack<size_t>& failed_processor_indexes,
            unordered_map<size_t, const Transition*>& applied_transitions,
            span<double>& reconfiguration_load,
            ReconfigurationTreeTraversionResult& result
        ) const;
    };

    class GreedyReconfigurationTable : public ReconfigurationTable
    {
    public:

        GreedyReconfigurationTable(
            size_t processor_count,
            span<double> normal_load,
            span<double> max_load,
            vector<TransitionSet> table
        ):
            ReconfigurationTable(
                processor_count,
                normal_load,
                max_load,
                table
            )
        { }

        StateVector reconfigure_state(const StateVector& sv1) const override;

    private:

        optional<Transition> update_reconfiguration_load(
            const StateVector& sv,
            Harray<double>& reconfiguration_load,
            const TransitionSet& transitions
        ) const;

        double load_score(const Harray<double>& load) const;
    };
}

module : private;

namespace sr
{
    void ReconfigurationTable::apply_transition_to_load(
        const Transition& transition,
        span<double> load,
        double sign
    ) const {
        for (const IdxL& increment : transition)
            load[increment.index] += sign * increment.load;
    }

    bool ReconfigurationTable::is_transition_valid(
        const StateVector& sv, const Transition& transition
    ) const {
        if (transition.empty()) return false;
        for (const auto& r : transition)
        {
            if (!sv.processors[r.index])
                return false;
        }
        return true;
    }

    bool ReconfigurationTable::is_transition_successful(
        const StateVector& original_sv,
        const Transition& transition,
        const Harray<double>& reconfiguration_load
    ) const {
        if (transition.empty()) return false;
        for (const auto& r : transition)
        {
            if (!original_sv.processors[r.index] || (reconfiguration_load[r.index] > max_load[r.index]))
                return false;
        }
        return true;
    }

    StateVector BruteForceReconfigurationTable::reconfigure_state(const StateVector& sv1) const
    {
        stack<size_t> failed_processors_indexes { };
        for (size_t i = 0; i < processor_count; i++)
            if (sv1.processors[i] == 0 && !table[i].empty())
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
            .best_zero_active_count = -1
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

        if (is_success)
            return result.best;
        else if (result.best_zero_active_count == -1)
            return sv1;
        else
            return result.best_zero;
    }

    bool BruteForceReconfigurationTable::traverse_reconfiguration_tree(
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
            }

            for (auto& [idx, applied_transition] : applied_transitions)
            {
                if (sv1.processors[idx] == 0 &&
                    is_transition_successful(sv1, *applied_transition, reconfiguration_load))
                {
                    sv2.processors[idx] = 1;
                }
            }

            if (sfunc(sv2))
            {
                result.best = move(sv2);
                return true;
            }
            else
            {
                long long current_active_count { static_cast<long long>(count(sv2.processors.begin(), sv2.processors.end(), true)) };
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
            if (!is_transition_valid(sv1, transition))
            {
                if (traverse_reconfiguration_tree(
                    sv1, failed_processor_indexes, applied_transitions, reconfiguration_load, result
                )) return true;
                else continue;
            }

            applied_transitions[current_processor_index] = &transition;
            apply_transition_to_load(transition, reconfiguration_load, 1.0);

            if (traverse_reconfiguration_tree(
                sv1, failed_processor_indexes,
                applied_transitions, reconfiguration_load, result
            )) return true;

            apply_transition_to_load(transition, reconfiguration_load, -1.0);
            applied_transitions.erase(current_processor_index);
        }
        failed_processor_indexes.push(current_processor_index);
        return false;
    }

    StateVector GreedyReconfigurationTable::reconfigure_state(const StateVector& sv1) const
    {
        Harray<double> reconfiguration_load { normal_load };

        unordered_map<size_t, optional<Transition>> transitions { };
        for (size_t i = 0; i < processor_count; i++)
            if (sv1.processors[i] == 0 && !table[i].empty())
                transitions[i] = update_reconfiguration_load(sv1, reconfiguration_load, table[i]);

        StateVector sv2 { sv1 };
        for (size_t i = 0; i < processor_count; i++)
        {
            if (sv1.processors[i] == 1 && reconfiguration_load[i] > max_load[i])
            {
                sv2.processors[i] = 0;
            }
            else if (transitions[i].has_value() && sv1.processors[i] == 0 &&
                     is_transition_successful(sv1, transitions[i].value(), reconfiguration_load))
            {
                sv2.processors[i] = 1;
            }
        }
        return sv2;
    }

    optional<Transition> GreedyReconfigurationTable::update_reconfiguration_load(
        const StateVector& sv,
        Harray<double>& reconfiguration_load, const TransitionSet& transitions
    ) const {
        unordered_map<double, const Transition*> score_transition { };
        for (const Transition& transition : transitions)
        {
            if (!is_transition_valid(sv, transition))
                continue;

            Harray<double> temp_load { reconfiguration_load };

            apply_transition_to_load(transition, temp_load.get_elements(), 1.0);

            double score { load_score(temp_load) };

            score_transition[score] = &transition;
        }

        auto best_transition = min_element(
            score_transition.begin(), score_transition.end(),
            [](const auto& a, const auto& b)
            {
                return a.first < b.first;
            }
        );

        if (best_transition != score_transition.end())
        {
            Transition result { move(*(*best_transition).second) };
            apply_transition_to_load(result, reconfiguration_load.get_elements(), 1.0);
            return result;
        }
        else
        {
            return nullopt;
        }
    }

    double GreedyReconfigurationTable::load_score(const Harray<double>& load) const
    {
        double result { 0 };
        for (size_t i = 0; i < processor_count; i++)
            result += load[i] / max_load[i];
        result /= static_cast<double>(processor_count);
        return result;
    }
}
