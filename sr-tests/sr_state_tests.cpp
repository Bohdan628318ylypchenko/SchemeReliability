#include "pch.h"

#include "CppUnitTest.h"

import sr_state;

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

		TEST_METHOD(harray_copy)
		{
			array<double, 4> arr1 { 1, 2, 3, 4 };
			Harray<double> harr1 { span<double> { arr1 } };

			Harray<double> harr2 { harr1 };
			harr1[0] = -1;

			Assert::AreEqual(-1.0, harr1[0]);
			Assert::AreEqual(1.0, harr2[0]);
		}

		TEST_METHOD(harray_move)
		{
			array<double, 4> arr1 { 1, 2, 3, 4 };
			Harray<double> harr1 { span<double> { arr1 } };

			Harray<double> harr2 { move(harr1) };
			harr2[0] = 100;

			Assert::AreEqual(100.0, harr2[0]);
		}

		TEST_METHOD(state_vector_copy)
		{
			array<bool, 4> arr1 { true, true, true, false };
			StateVector sv1 { span<bool> { arr1 }, 2 };

			StateVector sv2 { sv1 };
			sv2.all[0] = false;

			Assert::IsTrue(sv1.all[0]);
			Assert::IsTrue(sv1.processors[0]);
			Assert::IsFalse(sv2.all[0]);
			Assert::IsFalse(sv2.processors[0]);
		}

		TEST_METHOD(state_vector_move)
		{
			array<bool, 4> arr1 { true, true, true, false };
			StateVector sv1 { span<bool> { arr1 }, 2 };

			StateVector sv2 { move(sv1) };
			sv2.all[0] = false;

			Assert::IsFalse(sv2.all[0]);
		}
		
		TEST_METHOD(generate_full_state_vector_set)
		{
			const size_t all_count { 5 };
			const size_t processor_count { 2 };

			const size_t expected_full_state_set_size { 32 };

			StateVectorGenerator sg { all_count, processor_count };

			vector<StateVector> full_state_set { sg.generate_full_2n_state_vector_set() };

			Assert::AreEqual(static_cast<size_t>(32), full_state_set.size());
			for (auto& s : full_state_set)
			{
				Assert::AreEqual(all_count, s.all.size());
				Assert::AreEqual(processor_count, s.processors.size());
				Assert::IsTrue(s.all.data() == s.processors.data());
			}

			const bool expected_state_values[expected_full_state_set_size][all_count]
			{
				{ true, true, true, true, true },
				{ true, true, true, true, false },
				{ true, true, true, false, true },
				{ true, true, true, false, false },
				{ true, true, false, true, true },
				{ true, true, false, true, false },
				{ true, true, false, false, true },
				{ true, true, false, false, false },
				{ true, false, true, true, true },
				{ true, false, true, true, false },
				{ true, false, true, false, true },
				{ true, false, true, false, false },
				{ true, false, false, true, true },
				{ true, false, false, true, false },
				{ true, false, false, false, true },
				{ true, false, false, false, false },
				{ false, true, true, true, true },
				{ false, true, true, true, false },
				{ false, true, true, false, true },
				{ false, true, true, false, false },
				{ false, true, false, true, true },
				{ false, true, false, true, false },
				{ false, true, false, false, true },
				{ false, true, false, false, false },
				{ false, false, true, true, true },
				{ false, false, true, true, false },
				{ false, false, true, false, true },
				{ false, false, true, false, false },
				{ false, false, false, true, true },
				{ false, false, false, true, false },
				{ false, false, false, false, true },
				{ false, false, false, false, false }
			};

			for (size_t i = 0; i < expected_full_state_set_size; i++)
				for (size_t j = 0; j < all_count; j++)
					Assert::AreEqual(expected_state_values[i][j], full_state_set[i].all[j]);
		}
	};
}
