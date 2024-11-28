export module sr_reconfiguration_table;

export import sr_state;

import std;

using std::span;
using std::vector;
using std::accumulate;
using std::unordered_map;
using std::min_element;

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

    class ReconfigurationTable
    {
    private:

        const size_t processor_count;

        const Harray<double> normal_load;

        const Harray<double> max_load;

        const vector<vector<vector<IdxL>>> table;

    public:

        ReconfigurationTable(
            size_t processor_count,
            span<double> normal_load,
            span<double> max_load,
            vector<vector<vector<IdxL>>> table
        );

        ReconfigurationTable(const ReconfigurationTable&) = default;
        ReconfigurationTable& operator=(const ReconfigurationTable&) = default;

        ReconfigurationTable(ReconfigurationTable&&) = default;
        ReconfigurationTable& operator=(ReconfigurationTable&&) = default;

        inline size_t get_processor_count() const noexcept
        {
            return processor_count;
        }

        void reconfigure_state(const StateVector& sv1, StateVector& sv2) const;

    private:

        void update_reconfiguration_load(
            Harray<double>& reconfiguration_load,
            const vector<vector<IdxL>>& transitions
        ) const;

        void apply_transition_to_load(
            const vector<IdxL>& transition,
            Harray<double>& load
        ) const;

        double load_score(const Harray<double>& load) const;
    };
}

module : private;

namespace sr
{
    ReconfigurationTable::ReconfigurationTable(
        size_t processor_count,
        span<double> normal_load,
        span<double> max_load,
        vector<vector<vector<IdxL>>> table
    ):
        processor_count { processor_count },
        normal_load { normal_load },
        max_load { max_load },
        table { table }
    { }

    void ReconfigurationTable::reconfigure_state(const StateVector& sv1, StateVector& sv2) const
    {
        Harray<double> reconfiguration_load { normal_load };

        for (size_t i = 0; i < processor_count; i++)
            if (sv1.processors[i] == 0)
                update_reconfiguration_load(reconfiguration_load, table[i]);

        for (size_t i = 0; i < processor_count; i++)
            if (reconfiguration_load[i] > max_load[i])
                sv2.processors[i] = 0;
            else
                sv2.processors[i] = 1;
    }

    void ReconfigurationTable::update_reconfiguration_load(
        Harray<double>& reconfiguration_load, const vector<vector<IdxL>>& transitions
    ) const {
        unordered_map<double, const vector<IdxL>*> score_transition { };
        for (const vector<IdxL>& transition : transitions)
        {
            Harray<double> temp_load { reconfiguration_load };

            apply_transition_to_load(transition, temp_load);

            double score { load_score(temp_load) };

            score_transition[score] = &transition;
        }

        auto result = min_element(
            score_transition.begin(), score_transition.end(),
            [](const auto& a, const auto& b)
            {
                return a.first < b.first;
            }
        );

        if (result != score_transition.end())
            apply_transition_to_load(*(*result).second, reconfiguration_load);
    }

    void ReconfigurationTable::apply_transition_to_load(
        const vector<IdxL>& transition,
        Harray<double>& load
    ) const {
        for (const IdxL& increment : transition)
            load[increment.index] += increment.load;
    }

    double ReconfigurationTable::load_score(const Harray<double>& load) const
    {
        double result { 0 };
        for (size_t i = 0; i < processor_count; i++)
            result += load[i] / max_load[i];
        result /= static_cast<double>(processor_count);
        return result;
    }
}
