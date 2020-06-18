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
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a secret lock transaction body.
	template<typename THeader>
	struct SecretLockTransactionBody : public THeader {
	private:
		using TransactionType = SecretLockTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Secret_Lock, 1)

	public:
		/// Locked mosaic recipient address.
		UnresolvedAddress RecipientAddress;

		/// Secret.
		Hash256 Secret;

		/// Locked mosaic.
		UnresolvedMosaic Mosaic;

		/// Number of blocks for which a lock should be valid.
		BlockDuration Duration;

		/// Hash algorithm.
		LockHashAlgorithm HashAlgorithm;

	public:
		/// Calculates the real size of secret lock \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(SecretLock)

#pragma pack(pop)
}}
