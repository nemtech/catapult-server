/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/model/SecretProofTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/Alignment.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	using TransactionType = SecretProofTransaction;

#define TEST_CLASS SecretProofTransactionTests

	// region size + alignment + properties

#define TRANSACTION_FIELDS FIELD(RecipientAddress) FIELD(Secret) FIELD(ProofSize) FIELD(HashAlgorithm)

	namespace {
		template<typename T>
		void AssertTransactionHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize;

#define FIELD(X) expectedSize += sizeof(T::X);
			TRANSACTION_FIELDS
#undef FIELD

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 59u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasProperAlignment() {
#define FIELD(X) EXPECT_ALIGNED(T, X);
			TRANSACTION_FIELDS
#undef FIELD
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_Secret_Proof, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

#undef TRANSACTION_FIELDS

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(SecretProof)

	// endregion

	// region data pointers

	namespace {
		struct SecretProofTransactionTraits {
			static auto GenerateEntityWithAttachments(uint16_t proofSize) {
				uint32_t entitySize = sizeof(TransactionType) + proofSize;
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->ProofSize = proofSize;
				return pTransaction;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.ProofPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, SecretProofTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.ProofSize = 100;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 100, realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.ProofSize);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 0xFFFF, realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
