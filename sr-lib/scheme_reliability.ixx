export module scheme_reliability;

import :model;
import :algorithm;

export namespace sr
{
    template<size_t all_count, size_t processor_count>
    using StateVectorDto = sr_impl::model::StateVector<all_count, processor_count>;

    using TrUnit = sr_impl::model::IdxL;
    using TransitionDto = sr_impl::model::Transition;
    using TransitionSetDto = sr_impl::model::TransitionSet;

    using ElementDto = sr_impl::model::Element;
    using ProcessorDto = sr_impl::model::Processor;

    using sr_impl::model::StateVector;
    using sr_impl::model::SchemeFunction;

    template<size_t all_count, size_t processor_count>
    using SchemeDto = sr_impl::model::Scheme<all_count, processor_count>;

    using SchemeType = sr_impl::model::SchemeType;

    using SchemeReliabilitySummaryDto = sr_impl::model::SchemeReliabilitySummary;

    template<size_t all_count, size_t processor_count>
    inline SchemeReliabilitySummaryDto calculate_scheme_reliability(
        const SchemeDto<all_count, processor_count> scheme_dto
    ) {
        return sr_impl::algorithm::calculate_scheme_reliability<all_count, processor_count>(scheme_dto);
    }
}
