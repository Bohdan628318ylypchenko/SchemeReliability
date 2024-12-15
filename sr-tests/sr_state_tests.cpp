#include "pch.h"

#include "CppUnitTest.h"

import scheme_reliability;
using namespace sr;

import std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::vector;
using std::unique_ptr;
using std::span;
using std::array;
using std::move;

namespace sr::tests
{
	TEST_CLASS(StateTests)
	{
	public:

		TEST_METHOD(state_vector_copy)
		{
			constexpr size_t all_count { 4 };
			constexpr size_t processor_count { 2 };

			StateVectorDto<all_count, processor_count> sv1 { };
			sv1.all[0] = true;
			sv1.all[1] = true;
			sv1.all[2] = true;
			sv1.all[3] = false;

			StateVector<all_count, processor_count> sv2 { sv1 };
			sv2.all[0] = false;

			Assert::IsTrue(sv1.all[0]);
			Assert::IsTrue(sv1.processors[0]);
			Assert::IsFalse(sv2.all[0]);
			Assert::IsFalse(sv2.processors[0]);
		}

		TEST_METHOD(state_vector_move)
		{
			constexpr size_t all_count { 4 };
			constexpr size_t processor_count { 2 };

			StateVector<all_count, processor_count> sv1 { };

			StateVector sv2 { move(sv1) };
			sv2.all[0] = false;

			Assert::IsFalse(sv2.all[0]);
			Assert::IsFalse(sv2.processors[0]);
		}
	};
}
