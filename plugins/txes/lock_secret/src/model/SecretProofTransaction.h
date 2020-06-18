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

#pragma once
#include "LockHashAlgorithm.h"
#include "SecretLockEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a secret proof transaction body.
	template<typename THeader>
	struct SecretProofTransactionBody : public THeader {
	private:
		using TransactionType = SecretProofTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Secret_Proof, 1)

	public:
		/// Locked mosaic recipient address.
		UnresolvedAddress RecipientAddress;

		/// Secret.
		Hash256 Secret;

		/// Proof size in bytes.
		uint16_t ProofSize;

		/// Hash algorithm.
		LockHashAlgorithm HashAlgorithm;

		// followed by proof data if ProofSize != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Proof, uint8_t)

	private:
		template<typename T>
		static auto* ProofPtrT(T& transaction) {
			return transaction.ProofSize ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		/// Calculates the real size of secret proof \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.ProofSize;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(SecretProof)

#pragma pack(pop)
}}
