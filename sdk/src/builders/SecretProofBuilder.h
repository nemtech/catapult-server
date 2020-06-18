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
#include "TransactionBuilder.h"
#include "plugins/txes/lock_secret/src/model/SecretProofTransaction.h"

namespace catapult { namespace builders {

	/// Builder for a secret proof transaction.
	class SecretProofBuilder : public TransactionBuilder {
	public:
		using Transaction = model::SecretProofTransaction;
		using EmbeddedTransaction = model::EmbeddedSecretProofTransaction;

	public:
		/// Creates a secret proof builder for building a secret proof transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		SecretProofBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the locked mosaic recipient address to \a recipientAddress.
		void setRecipientAddress(const UnresolvedAddress& recipientAddress);

		/// Sets the secret to \a secret.
		void setSecret(const Hash256& secret);

		/// Sets the hash algorithm to \a hashAlgorithm.
		void setHashAlgorithm(model::LockHashAlgorithm hashAlgorithm);

		/// Sets the proof data to \a proof.
		void setProof(const RawBuffer& proof);

	public:
		/// Gets the size of secret proof transaction.
		/// \note This returns size of a normal transaction not embedded transaction.
		size_t size() const;

		/// Builds a new secret proof transaction.
		std::unique_ptr<Transaction> build() const;

		/// Builds a new embedded secret proof transaction.
		std::unique_ptr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		size_t sizeImpl() const;

		template<typename TTransaction>
		std::unique_ptr<TTransaction> buildImpl() const;

	private:
		UnresolvedAddress m_recipientAddress;
		Hash256 m_secret;
		model::LockHashAlgorithm m_hashAlgorithm;
		std::vector<uint8_t> m_proof;
	};
}}
