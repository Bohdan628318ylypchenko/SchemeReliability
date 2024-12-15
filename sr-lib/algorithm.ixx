module;

#include "readwritequeue.h"

export module scheme_reliability:algorithm;

import :model;
using namespace sr_impl::model;

using namespace moodycamel;

import std;
using std::pow;
using std::array;
using std::vector;
using std::span;
using std::stack;
using std::unordered_map;
using std::optional, std::nullopt;
using std::pair;
using std::ofstream;
using std::filesystem::path;
using std::filesystem::exists, std::filesystem::remove_all, std::filesystem::create_directory;
using std::runtime_error;
using std::unique_ptr, std::make_unique;
using std::string;
using std::format, std::println, std::cerr;
using std::vformat, std::make_format_args;
using std::thread;
using std::move;

namespace sr_impl::algorithm
{
    template<size_t all_count, size_t processor_count>
    class ReconfigurationTable
    {
        static_assert(
            processor_count < all_count,
            "processor count must be less or equal than elements count"
        );

    protected:

        array<double, processor_count> normal_load;
        array<double, processor_count> max_load;

        vector<TransitionSet> table;

        ReconfigurationTable(
            const Scheme<all_count, processor_count>& scheme
        ):
            normal_load { },
            max_load { },
            table(scheme.processors.size())
        {
            for (size_t i = 0; i < processor_count; i++)
            {
                normal_load[i] = scheme.processors[i].normal_load;
                max_load[i] = scheme.processors[i].max_load;
                table[i] = scheme.processors[i].transitions;
            }
        }

    public:

        ReconfigurationTable() = delete;
        virtual ~ReconfigurationTable() = default;

        ReconfigurationTable(const ReconfigurationTable&) = default;
        ReconfigurationTable& operator=(const ReconfigurationTable&) = default;

        ReconfigurationTable(ReconfigurationTable&&) = default;
        ReconfigurationTable& operator=(ReconfigurationTable&&) = default;

        virtual StateVector<all_count, processor_count> reconfigure_state(
            const StateVector<all_count, processor_count>& sv1
        ) const = 0;

    protected:

        void apply_transition_to_load(
            const Transition& transition,
            array<double, processor_count>& load,
            double sign
        ) const {
            for (const IdxL& increment : transition)
                load[increment.index] += sign * increment.load;
        }

        bool is_transition_valid(
            const StateVector<all_count, processor_count>& sv,
            const Transition& transition
        ) const {
            if (transition.empty()) return false;
            for (const auto& t : transition)
                if (!sv.processors[t.index]) return false;
            return true;
        }

        bool is_transition_successful(
            const StateVector<all_count, processor_count>& original_sv,
            const Transition& transition,
            const array<double, processor_count>& reconfiguration_load
        ) const {
            if (transition.empty()) return false;
            for (const auto& t : transition)
                if (!original_sv.processors[t.index] || (reconfiguration_load[t.index] > max_load[t.index]))
                    return false;
            return true;
        }
    };

    template<size_t all_count, size_t processor_count>
    class BruteForceReconfigurationTable : public ReconfigurationTable<all_count, processor_count>
    {
    private:

        const SchemeFunction<all_count, processor_count> scheme_function;

        struct ReconfigurationTreeTraversionResult
        {
            StateVector<all_count, processor_count> best;
            StateVector<all_count, processor_count> best_zero;
            long long best_zero_active_count;
        };

    public:

        BruteForceReconfigurationTable(
            const Scheme<all_count, processor_count>& scheme
        ):
            ReconfigurationTable<all_count, processor_count> { scheme },
            scheme_function { scheme.scheme_function }
        { }

        StateVector<all_count, processor_count> reconfigure_state(
            const StateVector<all_count, processor_count>& sv1
        ) const override {
            stack<size_t> failed_processors_indexes { };
            for (size_t i = 0; i < processor_count; i++)
                if (sv1.processors[i] == 0 && !this->table[i].empty())
                    failed_processors_indexes.push(i);
            if (failed_processors_indexes.empty())
                return sv1;

            unordered_map<size_t, const Transition*> applied_transitions { };
            array<double, processor_count> reconfiguration_load { this->normal_load };
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

    private:
        
        bool traverse_reconfiguration_tree(
            const StateVector<all_count, processor_count>& sv1,
            stack<size_t>& failed_processor_indexes,
            unordered_map<size_t, const Transition*>& applied_transitions,
            array<double, processor_count>& reconfiguration_load,
            ReconfigurationTreeTraversionResult& result
        ) const {
            if (failed_processor_indexes.empty())
            {
                StateVector sv2 { sv1 };

                for (size_t i = 0; i < processor_count; i++)
                {
                    if (sv1.processors[i] == 1 && reconfiguration_load[i] > this->max_load[i])
                    {
                        sv2.processors[i] = 0;
                    }
                }

                for (auto& [idx, applied_transition] : applied_transitions)
                {
                    if (sv1.processors[idx] == 0 &&
                        this->is_transition_successful(sv1, *applied_transition, reconfiguration_load))
                    {
                        sv2.processors[idx] = 1;
                    }
                }

                if (scheme_function(sv2))
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
            for (const Transition& transition : this->table[current_processor_index])
            {
                if (!this->is_transition_valid(sv1, transition))
                {
                    if (traverse_reconfiguration_tree(
                        sv1, failed_processor_indexes, applied_transitions, reconfiguration_load, result
                    )) return true;
                    else continue;
                }

                applied_transitions[current_processor_index] = &transition;
                this->apply_transition_to_load(transition, reconfiguration_load, 1.0);

                if (traverse_reconfiguration_tree(
                    sv1, failed_processor_indexes,
                    applied_transitions, reconfiguration_load, result
                )) return true;

                this->apply_transition_to_load(transition, reconfiguration_load, -1.0);
                applied_transitions.erase(current_processor_index);
            }
            failed_processor_indexes.push(current_processor_index);
            return false;
        }
    };

    template<size_t all_count, size_t processor_count>
    class GreedyReconfigurationTable : public ReconfigurationTable<all_count, processor_count>
    {
    private:

        static constexpr double OVERLOAD_COEFFICIENT { 1e6 };

    public:

        GreedyReconfigurationTable(
            const Scheme<all_count, processor_count>& scheme
        ):
            ReconfigurationTable<all_count, processor_count> { scheme }
        { }

        StateVector<all_count, processor_count> reconfigure_state(
            const StateVector<all_count, processor_count>& sv1
        ) const override {
            array<double, processor_count> reconfiguration_load { this->normal_load };

            unordered_map<size_t, optional<Transition>> transitions { };
            for (size_t i = 0; i < processor_count; i++)
                if (sv1.processors[i] == 0 && !this->table[i].empty())
                    transitions[i] = update_reconfiguration_load(sv1, reconfiguration_load, this->table[i]);
            if (transitions.empty())
                return sv1;

            StateVector<all_count, processor_count> sv2 { sv1 };
            for (size_t i = 0; i < processor_count; i++)
            {
                if (sv1.processors[i] == 1 && reconfiguration_load[i] > this->max_load[i])
                {
                    sv2.processors[i] = 0;
                }
                else if (transitions[i].has_value() && sv1.processors[i] == 0 &&
                         this->is_transition_successful(sv1, transitions[i].value(), reconfiguration_load))
                {
                    sv2.processors[i] = 1;
                }
            }
            return sv2;
        }

    private:

        optional<Transition> update_reconfiguration_load(
            const StateVector<all_count, processor_count>& sv,
            array<double, processor_count>& reconfiguration_load,
            const TransitionSet& transitions
        ) const {
            unordered_map<double, const Transition*> score_transition { };
            for (const Transition& transition : transitions)
            {
                if (!this->is_transition_valid(sv, transition))
                    continue;

                array<double, processor_count> temp_load { reconfiguration_load };

                this->apply_transition_to_load(transition, temp_load, 1.0);

                double score { load_score(temp_load, transition.size()) };

                score_transition[score] = &transition;
            }

            auto best_transition = min_element(
                score_transition.begin(), score_transition.end(),
                [](const pair<double, const Transition*>& a,
                   const pair<double, const Transition*>& b)
                {
                    return a.first < b.first;
                }
            );

            if (best_transition != score_transition.end())
            {
                Transition result { move(*(*best_transition).second) };
                this->apply_transition_to_load(result, reconfiguration_load, 1.0);
                return result;
            }
            else
            {
                return nullopt;
            }
        }

        double load_score(
            const array<double, processor_count>& load, size_t transition_size
        ) const {
            double result { 0 };
            for (size_t i = 0; i < processor_count; i++)
            {
                double temp { load[i] / this->max_load[i] };
                if (temp > 1.0) temp *= OVERLOAD_COEFFICIENT;
                result += temp;
            }
            result /= static_cast<double>(transition_size);
            return result;
        }
    };

    template<size_t all_count, size_t processor_count>
    class StateVectorProcessor
    {
    private:

        static constexpr size_t INITIAL_QUEUE_SIZE { 1024 };

        const ReconfigurationTable<all_count, processor_count>& reconfiguration_table;
        const span<double> p;
        const span<double> q;
        const SchemeFunction<all_count, processor_count> scheme_function;

        unique_ptr<char[]> buffer;
        ofstream data_file;
        bool is_end;
        thread processor_thread;

        ReaderWriterQueue<unique_ptr<StateVector<all_count, processor_count>>> queue;
        bool is_queue_empty;

        SchemeReliabilitySummary scheme_reliability_summary;

    public:

        StateVectorProcessor(
            const ReconfigurationTable<all_count, processor_count>& reconfiguration_table,
            const span<double> p,
            const span<double> q,
            const SchemeFunction<all_count, processor_count> scheme_function,
            path data_file_path,
            size_t buffer_size
        ):
            reconfiguration_table { reconfiguration_table },
            p { p }, q { q }, scheme_function { scheme_function },
            buffer { new char[buffer_size] },
            data_file { data_file_path, std::ios::binary },
            is_end { false },
            processor_thread { },
            queue(INITIAL_QUEUE_SIZE), is_queue_empty { false },
            scheme_reliability_summary
            {
                .sp = 0, .sq = 0,
                .state_vector_set_count = 0,
                .result_path = data_file_path
            }
        {
            if (!data_file.is_open())
            {
                string msg { format("Error: can't open data_file {} for writing", data_file_path.string()) };
                //println(cerr, msg);
                throw runtime_error(msg);
            }

            data_file.rdbuf()->pubsetbuf(buffer.get(), buffer_size);
        }

        inline const SchemeReliabilitySummary& get_scheme_reliability_summary() const
        {
            return scheme_reliability_summary;
        }

        void assign_for_processment(const StateVector<all_count, processor_count>& sv)
        {
            queue.enqueue(unique_ptr<StateVector<all_count, processor_count>>(new StateVector<all_count, processor_count>(sv)));
        }

        void start()
        {
            processor_thread = thread
            {
                [this]()
                {
                    while (!is_end || !is_queue_empty)
                    {
                        unique_ptr<StateVector<all_count, processor_count>> psv { };
                        if (queue.try_dequeue(psv))
                        {
                            is_queue_empty = false;
                        }
                        else
                        {
                            is_queue_empty = true;
                            continue;
                        }

                        const StateVector<all_count, processor_count>& sv1 { *psv };
                        StateVector<all_count, processor_count> sv2 { reconfiguration_table.reconfigure_state(sv1) };

                        bool scheme_state_sv1 { scheme_function(sv1) };
                        bool scheme_state_sv2 { scheme_function(sv2) };
                        bool scheme_state { scheme_state_sv1 || scheme_state_sv2 };
                        double probability { calculate_probability(sv1) };

                        ScoredStateVector<all_count, processor_count> ssv
                        {
                            .scheme_state_sv1 = scheme_state_sv1,
                            .scheme_state_sv2 = scheme_state_sv2,
                            .scheme_state = scheme_state,
                            .probability = probability,
                            .sv1 = sv1.sv,
                            .sv2 = sv2.sv
                        };
                        write_scored_state_vector(ssv);

                        if (scheme_state_sv2)
                            scheme_reliability_summary.sp += probability;
                        else
                            scheme_reliability_summary.sq += probability;

                        scheme_reliability_summary.state_vector_set_count++;
                    }
                }
            };
        }

        void join()
        {
            is_end = true;
            processor_thread.join();
        }

    private:

        void write_scored_state_vector(const ScoredStateVector<all_count, processor_count>& ssv)
        {
            data_file.write(reinterpret_cast<const char*>(&ssv.scheme_state_sv1), sizeof(bool));
            data_file.write(reinterpret_cast<const char*>(&ssv.scheme_state_sv2), sizeof(bool));
            data_file.write(reinterpret_cast<const char*>(&ssv.scheme_state), sizeof(bool));
            data_file.write(reinterpret_cast<const char*>(&ssv.probability), sizeof(double));
            data_file.write(reinterpret_cast<const char*>(ssv.sv1.data()), all_count * sizeof(bool));
            data_file.write(reinterpret_cast<const char*>(ssv.sv2.data()), all_count * sizeof(bool));
        }

        double calculate_probability(const StateVector<all_count, processor_count>& sv)
        {
            double result { 1.0 };
            for (size_t i = 0; i < sv.all.size(); i++)
                result *= (sv.all[i] * p[i] + (1 - sv.all[i]) * q[i]);
            return result;
        }
    };

    template<size_t all_count, size_t processor_count>
    class SchemeReliabilityCalculator
    {
        static_assert(
            processor_count < all_count,
            "processor count must be less or equal than elements count"
        );

    private:

        static constexpr size_t BUFFER_SIZE { 8192 };

        const string BINARY_SCORED_STATE_SET_DATA_EXTENSION { "ssv" };
        const string SCHEME_RELIABILITY_ELEMENTS_EXTENSION { "elems" };

        const string DATA_FILE_NAME_FORMAT { "{}/{}-{}.{}" };
        const string ELEMENTS_FILE_NAME_FORMAT { "{}/{}.{}" };

        const size_t full_state_vector_set_size;

        struct TraversionState
        {
            StateVector<all_count, processor_count> current_sv;
            size_t sv_processor_idx;
            vector<StateVectorProcessor<all_count, processor_count>>& sv_processors;
            size_t visited_state_vector_count;

            inline void update_sv_processor_idx()
            {
                sv_processor_idx = (sv_processor_idx + 1) % sv_processors.size();
            }
        };

    public:

        SchemeReliabilityCalculator():
            full_state_vector_set_size { static_cast<size_t>(pow(2, all_count)) }
        { }

        SchemeReliabilitySummary calculate_scheme_reliability(
            const Scheme<all_count, processor_count>& scheme
        ) {
            path scheme_result_path { scheme.scheme_name };
            if (exists(scheme_result_path))
                remove_all(scheme_result_path);
            create_directory(path(scheme.scheme_name));

            write_scheme_reliability_elements_ino(scheme);

            ReconfigurationTable<all_count, processor_count>* reconfiguration_table_memory;
            if (scheme.type == SchemeType::Brute)
                reconfiguration_table_memory = new BruteForceReconfigurationTable<all_count, processor_count>(scheme);
            else
                reconfiguration_table_memory = new GreedyReconfigurationTable<all_count, processor_count>(scheme);
            unique_ptr<ReconfigurationTable<all_count, processor_count>> reconfiguration_table { reconfiguration_table_memory };
            
            auto thread_count { thread::hardware_concurrency() };
            size_t buffer_size { BUFFER_SIZE };
            array<double, all_count> p { };
            array<double, all_count> q { };
            for (size_t i = 0; i < processor_count; i++)
            {
                p[i] = scheme.processors[i].p;
                q[i] = scheme.processors[i].q;
            }
            for (size_t i = processor_count; i < all_count; i++)
            {
                p[i] = scheme.elements[i - processor_count].p;
                q[i] = scheme.elements[i - processor_count].q;
            }
            vector<StateVectorProcessor<all_count, processor_count>> sv_processors { };
            sv_processors.reserve(thread_count);
            for (size_t i = 0; i < thread_count; i++)
            {
                sv_processors.push_back(StateVectorProcessor<all_count, processor_count> {
                    *reconfiguration_table,
                    p, q,
                    scheme.scheme_function,
                    path(
                        vformat(
                            this->DATA_FILE_NAME_FORMAT,
                            make_format_args(
                                scheme.scheme_name,
                                scheme.scheme_name, i,
                                this->BINARY_SCORED_STATE_SET_DATA_EXTENSION
                            )
                        )
                    ),
                    buffer_size
                });
            }

            TraversionState tstate
            {
                .current_sv = StateVector<all_count, processor_count>(),
                .sv_processor_idx = 0,
                .sv_processors = sv_processors,
                .visited_state_vector_count = 0
            };
            for (StateVectorProcessor<all_count, processor_count>& sv_processor : sv_processors)
                sv_processor.start();
            traverse_state_vector_tree(0, tstate);
            for (StateVectorProcessor<all_count, processor_count>& sv_processor : sv_processors)
                sv_processor.join();

            SchemeReliabilitySummary result
            {
                .sp = 0,
                .sq = 0,
                .state_vector_set_count = 0,
                .result_path = scheme_result_path
            };
            for (const StateVectorProcessor<all_count, processor_count>& sv_processor : sv_processors)
            {
                result.sp += sv_processor.get_scheme_reliability_summary().sp;
                result.sq += sv_processor.get_scheme_reliability_summary().sq;
                result.state_vector_set_count += sv_processor.get_scheme_reliability_summary().state_vector_set_count;
            }

            return result;
        }

    private:

        void write_scheme_reliability_elements_ino(
            const Scheme<all_count, processor_count>& scheme
        ) const {
            auto element_file_path = path(
                vformat(
                    ELEMENTS_FILE_NAME_FORMAT,
                    make_format_args(
                        scheme.scheme_name,
                        scheme.scheme_name,
                        SCHEME_RELIABILITY_ELEMENTS_EXTENSION
                    )
                )
            );
            ofstream elements_file { element_file_path, std::ios::trunc };
            if (!elements_file.is_open())
            {
                string msg { format("Error: can't open elements_file {} for writing", element_file_path.string()) };
                //println(cerr, msg);
                throw runtime_error(msg);
            }

            for (const Processor& element : scheme.processors)
            {
                elements_file << element.name << ",";
            }

            elements_file << scheme.elements.front().name;
            for (auto it = scheme.elements.begin() + 1; it != scheme.elements.end(); it++)
            {
                elements_file << "," << it->name;
            }
        }

        void traverse_state_vector_tree(
            size_t element_idx,
            TraversionState& tstate
        ) const {
            if (element_idx == tstate.current_sv.all.size())
            {
                tstate.sv_processors[tstate.sv_processor_idx].assign_for_processment(tstate.current_sv);
                tstate.update_sv_processor_idx();
                tstate.visited_state_vector_count++;
                return;
            }
            else
            {
                tstate.current_sv.all[element_idx] = 1;
                traverse_state_vector_tree(
                    element_idx + 1,
                    tstate
                );

                tstate.current_sv.all[element_idx] = 0;
                traverse_state_vector_tree(
                    element_idx + 1,
                    tstate
                );
            }
        }
    };

    template<size_t all_count, size_t processor_count>
    SchemeReliabilitySummary calculate_scheme_reliability(
        const Scheme<all_count, processor_count>& scheme
    ) {
        SchemeReliabilityCalculator<all_count, processor_count> scheme_reliability_calculator { };
        return scheme_reliability_calculator.calculate_scheme_reliability(scheme);
    }
}
