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

#include "FilechainTestUtils.h"
#include "RealTransactionFactory.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nemesis/NemesisCompatibleConfiguration.h"
#include "tests/test/nodeps/TestNetworkConstants.h"

namespace catapult { namespace test {

	std::vector<crypto::KeyPair> GetNemesisKeyPairs() {
		std::vector<crypto::KeyPair> nemesisKeyPairs;
		for (const auto* pRecipientPrivateKeyString : test::Test_Network_Private_Keys)
			nemesisKeyPairs.push_back(crypto::KeyPair::FromString(pRecipientPrivateKeyString));

		return nemesisKeyPairs;
	}

	BlockWithAttributes CreateBlock(
			const std::vector<crypto::KeyPair>& nemesisKeyPairs,
			const Address& recipientAddress,
			std::mt19937_64& rnd,
			uint64_t height,
			const utils::TimeSpan& timeSpacing) {
		auto numNemesisAccounts = nemesisKeyPairs.size();

		std::uniform_int_distribution<size_t> numTransactionsDistribution(5, 20);
		auto numTransactions = numTransactionsDistribution(rnd);

		BlockWithAttributes blockWithAttributes;
		test::ConstTransactions transactions;
		std::uniform_int_distribution<size_t> accountIndexDistribution(0, numNemesisAccounts - 1);
		for (auto i = 0u; i < numTransactions; ++i) {
			auto senderIndex = accountIndexDistribution(rnd);
			const auto& sender = nemesisKeyPairs[senderIndex];

			std::uniform_int_distribution<Amount::ValueType> amountDistribution(1000, 10 * 1000);
			Amount amount(amountDistribution(rnd) * 1'000'000);
			auto pTransaction = test::CreateUnsignedTransferTransaction(
					sender.publicKey(),
					extensions::CopyToUnresolvedAddress(recipientAddress),
					amount);
			pTransaction->MaxFee = Amount(0);
			transactions.push_back(std::move(pTransaction));

			blockWithAttributes.SenderIds.push_back(senderIndex);
			blockWithAttributes.Amounts.push_back(amount);
		}

		auto harvesterIndex = accountIndexDistribution(rnd);
		auto pBlock = test::GenerateBlockWithTransactions(nemesisKeyPairs[harvesterIndex], transactions);
		pBlock->Height = Height(height);
		pBlock->Difficulty = Difficulty(Difficulty().unwrap() + height);
		pBlock->Timestamp = Timestamp(height * timeSpacing.millis());

		blockWithAttributes.pBlock = std::move(pBlock);
		return blockWithAttributes;
	}
}}
