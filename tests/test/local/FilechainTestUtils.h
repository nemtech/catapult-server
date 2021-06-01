/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "catapult/config/CatapultConfiguration.h"
#include "catapult/crypto/KeyPair.h"
#include <random>

namespace catapult { namespace model { struct Block; } }

namespace catapult { namespace test {

	/// Gets the test nemesis block key pairs.
	std::vector<crypto::KeyPair> GetNemesisKeyPairs();

	/// Block with additional metadata.
	struct BlockWithAttributes {
	public:
		/// Block.
		std::unique_ptr<model::Block> pBlock;

		/// Ids of senders
		std::vector<size_t> SenderIds;

		/// Amounts of currency in transactions.
		std::vector<Amount> Amounts;
	};

	/// Creates a random block at \a height using \a rng as a random source
	/// with transactions from \a nemesisKeyPairs to \a recipientAddress and optional \a timeSpacing.
	BlockWithAttributes CreateBlock(
			const std::vector<crypto::KeyPair>& nemesisKeyPairs,
			const Address& recipientAddress,
			std::mt19937_64& rng,
			uint64_t height,
			const utils::TimeSpan& timeSpacing = utils::TimeSpan::FromMinutes(3));
}}
