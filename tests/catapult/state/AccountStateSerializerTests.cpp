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

#include "catapult/state/AccountStateSerializer.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/Casting.h"
#include "tests/test/core/AccountStateTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS AccountStateSerializerTests

	namespace {
		constexpr uint8_t Regular_Format_Tag = 0;
		constexpr uint8_t High_Value_Format_Tag = 1;

		size_t GetManyMosaicsCount() {
			return test::GetStressIterationCount() ? 65535 : 1000;
		}

		// region raw structures

#pragma pack(push, 1)

		struct PackedImportanceSnapshot {
			catapult::Importance Importance;
			model::ImportanceHeight Height;
		};

		struct PackedActivityBucket {
			model::ImportanceHeight StartHeight;
			Amount TotalFeesPaid;
			uint32_t BeneficiaryCount;
			uint64_t RawScore;
		};

		struct AccountStateHeader {
			catapult::Address Address;
			Height AddressHeight;
			Key PublicKey;
			Height PublicKeyHeight;

			state::AccountType AccountType;
			uint8_t Format;
			AccountPublicKeys::KeyType SupplementalPublicKeysMask;
			uint8_t NumVotingKeys;
		};

		struct HighValueImportanceHeader {
			PackedImportanceSnapshot Snapshot;
			PackedActivityBucket Buckets[5];
		};

		struct MosaicHeader {
			uint16_t MosaicsCount;
		};

		struct HistoricalRegularHeader {
			PackedImportanceSnapshot HistoricalSnapshots[3];
			PackedActivityBucket HistoricalBuckets[7];
		};

		struct HistoricalHighValueHeader {
			PackedImportanceSnapshot HistoricalSnapshots[2];
			PackedActivityBucket HistoricalBuckets[2];
		};

#pragma pack(pop)

		// endregion

		// region account state utils

		struct RandomSeed {
			AccountPublicKeys::KeyType SupplementalPublicKeysMask = AccountPublicKeys::KeyType::Unset;
			uint8_t NumVotingKeys = 0;
			bool ShouldConstrainOptimizedMosaicId = false;
		};

		AccountState CreateRandomAccountState(size_t numMosaics, const RandomSeed& seed) {
			auto accountState = AccountState(test::GenerateRandomAddress(), Height(123));
			test::FillWithRandomData(accountState.PublicKey);
			accountState.PublicKeyHeight = Height(234);

			accountState.AccountType = static_cast<AccountType>(33);
			test::SetRandomSupplementalPublicKeys(accountState, seed.SupplementalPublicKeysMask, seed.NumVotingKeys);

			test::RandomFillAccountData(1, accountState, numMosaics);
			if (seed.ShouldConstrainOptimizedMosaicId) {
				if (0 != numMosaics) {
					auto iter = accountState.Balances.begin();
					std::advance(iter, static_cast<int64_t>(test::Random() % numMosaics));
					accountState.Balances.optimize(iter->first);
				}
			} else {
				accountState.Balances.optimize(test::GenerateRandomValue<MosaicId>());
			}

			return accountState;
		}

		auto CopySnapshots(const AccountState& accountState) {
			std::array<AccountImportanceSnapshots::ImportanceSnapshot, Importance_History_Size> copy;
			std::copy(accountState.ImportanceSnapshots.begin(), accountState.ImportanceSnapshots.end(), copy.begin());
			return copy;
		}

		auto CopyBuckets(const AccountState& accountState) {
			std::array<AccountActivityBuckets::ActivityBucket, Activity_Bucket_History_Size> copy;
			std::copy(accountState.ActivityBuckets.begin(), accountState.ActivityBuckets.end(), copy.begin());
			return copy;
		}

		template<typename TSnapshot1, typename TSnapshot2>
		void CopySnapshotTo(const TSnapshot1& source, TSnapshot2& dest) {
			dest.Importance = source.Importance;
			dest.Height = source.Height;
		}

		template<typename TBucket1, typename TBucket2>
		void CopyBucketTo(const TBucket1& source, TBucket2& dest) {
			dest.StartHeight = source.StartHeight;
			dest.TotalFeesPaid = source.TotalFeesPaid;
			dest.BeneficiaryCount = source.BeneficiaryCount;
			dest.RawScore = source.RawScore;
		}

		template<typename TSnapshot>
		void PushSnapshot(AccountState& accountState, const TSnapshot& snapshot) {
			accountState.ImportanceSnapshots.set(snapshot.Importance, snapshot.Height);
		}

		template<typename TBucket>
		void PushBucket(AccountState& accountState, const TBucket& bucket) {
			accountState.ActivityBuckets.update(bucket.StartHeight, [&bucket](auto& accountStateBucket) {
				accountStateBucket.TotalFeesPaid = bucket.TotalFeesPaid;
				accountStateBucket.BeneficiaryCount = bucket.BeneficiaryCount;
				accountStateBucket.RawScore = bucket.RawScore;
			});
		}

		void ClearSnapshotsAndBuckets(AccountState& accountState) {
			while (model::ImportanceHeight() != accountState.ImportanceSnapshots.height())
				accountState.ImportanceSnapshots.pop();

			while (model::ImportanceHeight() != accountState.ActivityBuckets.begin()->StartHeight)
				accountState.ActivityBuckets.pop();
		}

		// endregion

		// region header => account state utils

		AccountState CreateAccountStateFromHeader(const AccountStateHeader& header) {
			auto accountState = AccountState(header.Address, header.AddressHeight);
			accountState.PublicKey = header.PublicKey;
			accountState.PublicKeyHeight = header.PublicKeyHeight;

			accountState.AccountType = header.AccountType;
			return accountState;
		}

		void ProcessSnapshots(AccountState& accountState, const PackedImportanceSnapshot* pSnapshots, size_t count) {
			for (auto i = count; i > 0; --i)
				PushSnapshot(accountState, pSnapshots[i - 1]);
		}

		void ProcessBuckets(AccountState& accountState, const PackedActivityBucket* pBuckets, size_t count) {
			for (auto i = count; i > 0; --i)
				PushBucket(accountState, pBuckets[i - 1]);
		}

		void ProcessHighValueImportanceHeader(AccountState& accountState, const HighValueImportanceHeader& header) {
			PushSnapshot(accountState, header.Snapshot);
			ProcessBuckets(accountState, header.Buckets, CountOf(header.Buckets));
		}

		void ProcessMosaicHeader(AccountState& accountState, const MosaicHeader& header) {
			const auto* pMosaicData = reinterpret_cast<const uint8_t*>(&header + 1);

			for (auto i = 0u; i < header.MosaicsCount; ++i, pMosaicData += sizeof(model::Mosaic)) {
				model::Mosaic mosaic;
				std::memcpy(reinterpret_cast<void*>(&mosaic), pMosaicData, sizeof(model::Mosaic));

				accountState.Balances.credit(mosaic.MosaicId, mosaic.Amount);

				if (0 == i)
					accountState.Balances.optimize(mosaic.MosaicId);
			}
		}

		size_t SetPublicKeyFromData(AccountPublicKeys::PublicKeyAccessor<Key>& publicKeyAccessor, const uint8_t* pData) {
			publicKeyAccessor.set(reinterpret_cast<const Key&>(*pData));
			return Key::Size;
		}

		size_t AddPublicKeyFromData(
				AccountPublicKeys::PublicKeysAccessor<model::PinnedVotingKey>& publicKeysAccessor,
				const uint8_t* pData) {
			publicKeysAccessor.add(reinterpret_cast<const model::PinnedVotingKey&>(*pData));
			return model::PinnedVotingKey::Size;
		}

		AccountState DeserializeNonHistoricalFromBuffer(const uint8_t* pData, uint8_t format) {
			// 1. process AccountStateHeader
			const auto& accountStateHeader = reinterpret_cast<const AccountStateHeader&>(*pData);
			pData += sizeof(AccountStateHeader);
			auto accountState = CreateAccountStateFromHeader(accountStateHeader);

			if (HasFlag(AccountPublicKeys::KeyType::Linked, accountStateHeader.SupplementalPublicKeysMask))
				pData += SetPublicKeyFromData(accountState.SupplementalPublicKeys.linked(), pData);

			if (HasFlag(AccountPublicKeys::KeyType::Node, accountStateHeader.SupplementalPublicKeysMask))
				pData += SetPublicKeyFromData(accountState.SupplementalPublicKeys.node(), pData);

			if (HasFlag(AccountPublicKeys::KeyType::VRF, accountStateHeader.SupplementalPublicKeysMask))
				pData += SetPublicKeyFromData(accountState.SupplementalPublicKeys.vrf(), pData);

			for (auto i = 0u; i < accountStateHeader.NumVotingKeys; ++i)
				pData += AddPublicKeyFromData(accountState.SupplementalPublicKeys.voting(), pData);

			if (High_Value_Format_Tag == format) {
				// 2. process HighValueImportanceHeader
				const auto& importanceHeader = reinterpret_cast<const HighValueImportanceHeader&>(*pData);
				pData += sizeof(HighValueImportanceHeader);
				ProcessHighValueImportanceHeader(accountState, importanceHeader);
			}

			// 3. process MosaicHeader and following mosaics
			const auto& mosaicHeader = reinterpret_cast<const MosaicHeader&>(*pData);
			pData += sizeof(MosaicHeader);
			ProcessMosaicHeader(accountState, mosaicHeader);

			// 4. sanity checks
			EXPECT_EQ(format, accountStateHeader.Format);
			return accountState;
		}

		// endregion

		// region account state => header utils

		size_t SetPublicKeyInData(const AccountPublicKeys::PublicKeyAccessor<Key>& publicKeyAccessor, uint8_t* pData) {
			reinterpret_cast<Key&>(*pData) = publicKeyAccessor.get();
			return Key::Size;
		}

		size_t SetPublicKeysInData(
				const AccountPublicKeys::PublicKeysAccessor<model::PinnedVotingKey>& publicKeysAccessor,
				uint8_t* pData) {
			for (auto i = 0u; i < publicKeysAccessor.size(); ++i) {
				reinterpret_cast<model::PinnedVotingKey&>(*pData) = publicKeysAccessor.get(i);
				pData += model::PinnedVotingKey::Size;
			}

			return publicKeysAccessor.size() * model::PinnedVotingKey::Size;
		}

		void SerializeNonHistoricalToBuffer(const AccountState& accountState, uint8_t format, std::vector<uint8_t>& buffer) {
			auto* pData = buffer.data();

			auto& accountStateHeader = reinterpret_cast<AccountStateHeader&>(*pData);
			accountStateHeader.Address = accountState.Address;
			accountStateHeader.AddressHeight = accountState.AddressHeight;
			accountStateHeader.PublicKey = accountState.PublicKey;
			accountStateHeader.PublicKeyHeight = accountState.PublicKeyHeight;
			accountStateHeader.AccountType = accountState.AccountType;
			accountStateHeader.Format = format;
			accountStateHeader.SupplementalPublicKeysMask = accountState.SupplementalPublicKeys.mask();
			accountStateHeader.NumVotingKeys = static_cast<uint8_t>(accountState.SupplementalPublicKeys.voting().size());
			pData += sizeof(AccountStateHeader);

			if (HasFlag(AccountPublicKeys::KeyType::Linked, accountStateHeader.SupplementalPublicKeysMask))
				pData += SetPublicKeyInData(accountState.SupplementalPublicKeys.linked(), pData);

			if (HasFlag(AccountPublicKeys::KeyType::Node, accountStateHeader.SupplementalPublicKeysMask))
				pData += SetPublicKeyInData(accountState.SupplementalPublicKeys.node(), pData);

			if (HasFlag(AccountPublicKeys::KeyType::VRF, accountStateHeader.SupplementalPublicKeysMask))
				pData += SetPublicKeyInData(accountState.SupplementalPublicKeys.vrf(), pData);

			pData += SetPublicKeysInData(accountState.SupplementalPublicKeys.voting(), pData);

			if (High_Value_Format_Tag == format) {
				auto& importanceHeader = reinterpret_cast<HighValueImportanceHeader&>(*pData);
				importanceHeader.Snapshot.Importance = accountState.ImportanceSnapshots.current();
				importanceHeader.Snapshot.Height = accountState.ImportanceSnapshots.height();

				auto accountStateBucketIter = accountState.ActivityBuckets.begin();
				for (auto& bucket : importanceHeader.Buckets) {
					CopyBucketTo(*accountStateBucketIter, bucket);
					++accountStateBucketIter;
				}

				pData += sizeof(HighValueImportanceHeader);
			}

			auto& mosaicHeader = reinterpret_cast<MosaicHeader&>(*pData);
			mosaicHeader.MosaicsCount = static_cast<uint16_t>(accountState.Balances.size());
			pData += sizeof(MosaicHeader);

			// pData is not 8-byte aligned, so need to use memcpy
			for (const auto& pair : accountState.Balances) {
				std::memcpy(pData, &pair.first, sizeof(uint64_t));
				pData += sizeof(uint64_t);

				std::memcpy(pData, &pair.second, sizeof(uint64_t));
				pData += sizeof(uint64_t);
			}
		}

		// endregion

		// region traits (regular)

		struct BasicRegularTraits {
			static constexpr size_t Mosaic_Header_Offset = sizeof(AccountStateHeader);

			static void CoerceToDesiredFormat(AccountState& accountState) {
				// push a zero importance to indicate a regular account
				auto nextHeight = accountState.ImportanceSnapshots.height() + model::ImportanceHeight(1);
				accountState.ImportanceSnapshots.set(Importance(), nextHeight);
			}
		};

		struct RegularNonHistoricalTraits : public BasicRegularTraits {
			using Serializer = AccountStateNonHistoricalSerializer;

			static size_t CalculatePackedSize(const AccountState& accountState) {
				auto size = sizeof(AccountStateHeader) + sizeof(MosaicHeader) + accountState.Balances.size() * sizeof(model::Mosaic);

				auto accountPublicKeysMask = accountState.SupplementalPublicKeys.mask();
				std::vector<std::pair<AccountPublicKeys::KeyType, size_t>> keySizes{
					{ AccountPublicKeys::KeyType::Linked, Key::Size },
					{ AccountPublicKeys::KeyType::Node, Key::Size },
					{ AccountPublicKeys::KeyType::VRF, Key::Size }
				};

				for (const auto& keySizePair : keySizes) {
					if (HasFlag(keySizePair.first, accountPublicKeysMask))
						size += keySizePair.second;
				}

				size += accountState.SupplementalPublicKeys.voting().size() * sizeof(model::PinnedVotingKey);
				return size;
			}

			static AccountState DeserializeFromBuffer(const uint8_t* pData) {
				return DeserializeNonHistoricalFromBuffer(pData, Regular_Format_Tag);
			}

			static std::vector<uint8_t> CopyToBuffer(const AccountState& accountState) {
				std::vector<uint8_t> buffer(CalculatePackedSize(accountState));
				SerializeNonHistoricalToBuffer(accountState, Regular_Format_Tag, buffer);
				return buffer;
			}

			static void AssertEqual(const AccountState& expected, const AccountState& actual) {
				// preprocess expected before comparing it to actual
				// 1. regular non-historical serialization doesn't save any snapshots (top importance is zero)
				// 2. regular non-historical serialization doesn't save any activity buckets
				auto expectedCopy = expected;
				ClearSnapshotsAndBuckets(expectedCopy);

				test::AssertEqual(expectedCopy, actual);
			}
		};

		struct RegularHistoricalTraits : public BasicRegularTraits {
			using Serializer = AccountStateSerializer;

			static size_t CalculatePackedSize(const AccountState& accountState) {
				return RegularNonHistoricalTraits::CalculatePackedSize(accountState)
						+ 3 * sizeof(PackedImportanceSnapshot)
						+ 7 * sizeof(PackedActivityBucket);
			}

			static AccountState DeserializeFromBuffer(const uint8_t* pData) {
				// 1. process non-historical data
				auto accountState = RegularNonHistoricalTraits::DeserializeFromBuffer(pData);
				pData += RegularNonHistoricalTraits::CalculatePackedSize(accountState);

				// 2. process HistoricalRegularHeader
				const auto& historicalHeader = reinterpret_cast<const HistoricalRegularHeader&>(*pData);
				pData += sizeof(HistoricalRegularHeader);
				ProcessSnapshots(accountState, historicalHeader.HistoricalSnapshots, CountOf(historicalHeader.HistoricalSnapshots));
				ProcessBuckets(accountState, historicalHeader.HistoricalBuckets, CountOf(historicalHeader.HistoricalBuckets));
				return accountState;
			}

			static std::vector<uint8_t> CopyToBuffer(const AccountState& accountState) {
				auto buffer = RegularNonHistoricalTraits::CopyToBuffer(accountState);
				buffer.resize(CalculatePackedSize(accountState));

				auto* pData = buffer.data() + RegularNonHistoricalTraits::CalculatePackedSize(accountState);
				for (const auto& snapshot : accountState.ImportanceSnapshots) {
					CopySnapshotTo(snapshot, reinterpret_cast<PackedImportanceSnapshot&>(*pData));
					pData += sizeof(PackedImportanceSnapshot);
				}

				for (const auto& bucket : accountState.ActivityBuckets) {
					CopyBucketTo(bucket, reinterpret_cast<PackedActivityBucket&>(*pData));
					pData += sizeof(PackedActivityBucket);
				}

				return buffer;
			}

			static void AssertEqual(const AccountState& expected, const AccountState& actual) {
				test::AssertEqual(expected, actual);
			}
		};

		// endregion

		// region traits (high value)

		template<typename TContainer, typename TPush>
		void ReapplyNonHistoricalValues(AccountState& accountState, const TContainer& values, TPush push) {
			for (auto i = 0u; i < values.size() - Rollback_Buffer_Size; ++i)
				push(accountState, values[values.size() - Rollback_Buffer_Size - 1 - i]);
		}

		struct BasicHighValueTraits {
			static constexpr size_t Mosaic_Header_Offset = sizeof(AccountStateHeader) + sizeof(HighValueImportanceHeader);

			static void CoerceToDesiredFormat(const AccountState&)
			{}
		};

		struct HighValueNonHistoricalTraits : public BasicHighValueTraits {
			using Serializer = AccountStateNonHistoricalSerializer;

			static size_t CalculatePackedSize(const AccountState& accountState) {
				return RegularNonHistoricalTraits::CalculatePackedSize(accountState) + sizeof(HighValueImportanceHeader);
			}

			static AccountState DeserializeFromBuffer(const uint8_t* pData) {
				return DeserializeNonHistoricalFromBuffer(pData, High_Value_Format_Tag);
			}

			static std::vector<uint8_t> CopyToBuffer(const AccountState& accountState) {
				std::vector<uint8_t> buffer(CalculatePackedSize(accountState));
				SerializeNonHistoricalToBuffer(accountState, High_Value_Format_Tag, buffer);
				return buffer;
			}

			static void AssertEqual(const AccountState& expected, const AccountState& actual) {
				// preprocess expected before comparing it to actual
				// 1. high value non-historical serialization doesn't save any rollback buffer snapshots
				// 2. high value non-historical serialization doesn't save any rollback buffer activity buckets
				auto expectedCopy = expected;
				auto snapshots = CopySnapshots(expectedCopy);
				auto buckets = CopyBuckets(expectedCopy);

				ClearSnapshotsAndBuckets(expectedCopy);

				ReapplyNonHistoricalValues(expectedCopy, snapshots, PushSnapshot<AccountImportanceSnapshots::ImportanceSnapshot>);
				ReapplyNonHistoricalValues(expectedCopy, buckets, PushBucket<AccountActivityBuckets::ActivityBucket>);

				test::AssertEqual(expectedCopy, actual);
			}
		};

		struct HighValueHistoricalTraits : public BasicHighValueTraits {
			using Serializer = AccountStateSerializer;

			static size_t CalculatePackedSize(const AccountState& accountState) {
				return RegularHistoricalTraits::CalculatePackedSize(accountState);
			}

			static AccountState DeserializeFromBuffer(const uint8_t* pData) {
				// 1. process non-historical data
				auto accountState = HighValueNonHistoricalTraits::DeserializeFromBuffer(pData);
				pData += HighValueNonHistoricalTraits::CalculatePackedSize(accountState);

				// 2. copy non historical importance information
				auto nonHistoricalSnapshots = CopySnapshots(accountState);
				auto nonHistoricalBuckets = CopyBuckets(accountState);
				ClearSnapshotsAndBuckets(accountState);

				// 3. process HistoricalHighValueHeader
				const auto& historicalHeader = reinterpret_cast<const HistoricalHighValueHeader&>(*pData);
				pData += sizeof(HistoricalHighValueHeader);
				ProcessSnapshots(accountState, historicalHeader.HistoricalSnapshots, CountOf(historicalHeader.HistoricalSnapshots));
				ProcessBuckets(accountState, historicalHeader.HistoricalBuckets, CountOf(historicalHeader.HistoricalBuckets));

				// 4. reapply non historical importance information
				constexpr auto PushImportanceSnapshot = PushSnapshot<AccountImportanceSnapshots::ImportanceSnapshot>;
				ReapplyNonHistoricalValues(accountState, nonHistoricalSnapshots, PushImportanceSnapshot);
				ReapplyNonHistoricalValues(accountState, nonHistoricalBuckets, PushBucket<AccountActivityBuckets::ActivityBucket>);
				return accountState;
			}

			static std::vector<uint8_t> CopyToBuffer(const AccountState& accountState) {
				auto buffer = HighValueNonHistoricalTraits::CopyToBuffer(accountState);
				buffer.resize(CalculatePackedSize(accountState));

				auto i = 0u;
				auto* pData = buffer.data() + HighValueNonHistoricalTraits::CalculatePackedSize(accountState);
				for (const auto& snapshot : accountState.ImportanceSnapshots) {
					if (i++ < Importance_History_Size - Rollback_Buffer_Size)
						continue;

					CopySnapshotTo(snapshot, reinterpret_cast<PackedImportanceSnapshot&>(*pData));
					pData += sizeof(PackedImportanceSnapshot);
				}

				i = 0;
				for (const auto& bucket : accountState.ActivityBuckets) {
					if (i++ < Activity_Bucket_History_Size - Rollback_Buffer_Size)
						continue;

					CopyBucketTo(bucket, reinterpret_cast<PackedActivityBucket&>(*pData));
					pData += sizeof(PackedActivityBucket);
				}

				return buffer;
			}

			static void AssertEqual(const AccountState& expected, const AccountState& actual) {
				test::AssertEqual(expected, actual);
			}
		};

		// endregion
	}

#define SERIALIZER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_RegularNonHistorical) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularNonHistoricalTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_RegularHistorical) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularHistoricalTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_HighValueNonHistorical) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<HighValueNonHistoricalTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_HighValueHistorical) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<HighValueHistoricalTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region Save

	namespace {
		void ForEachRandomSeed(const consumer<const RandomSeed&>& action) {
			auto accountPublicKeysMasks = std::initializer_list<AccountPublicKeys::KeyType>{
				AccountPublicKeys::KeyType::Unset,
				AccountPublicKeys::KeyType::Linked,
				AccountPublicKeys::KeyType::Node,
				AccountPublicKeys::KeyType::VRF,
				AccountPublicKeys::KeyType::Linked | AccountPublicKeys::KeyType::Node,
				AccountPublicKeys::KeyType::All
			};

			for (auto keyType : accountPublicKeysMasks) {
				for (auto numVotingKeys : std::initializer_list<uint8_t>{ 0, 3 }) {
					for (auto optimizedMosaicIdConstraint : { true, false }) {
						CATAPULT_LOG(debug)
								<< "key type mask: " << static_cast<uint16_t>(keyType)
								<< ", num voting keys: " << static_cast<uint16_t>(numVotingKeys)
								<< ", should constrain optimized mosaic id: " << optimizedMosaicIdConstraint;

						action({ keyType, numVotingKeys, optimizedMosaicIdConstraint });
					}
				}
			}
		}

		template<typename TTraits>
		void AssertEqual(const RandomSeed& seed, const AccountState& expected, AccountState&& actual) {
			// adjust actual's optimizedMosaicId to match expected's when it does not contain the optimized mosaic id
			if (0 == expected.Balances.size() || !seed.ShouldConstrainOptimizedMosaicId) {
				if (0 == expected.Balances.size()) {
					// optimized mosaic id will be zero if no mosaics are present
					EXPECT_EQ(MosaicId(), actual.Balances.optimizedMosaicId());
				} else {
					// optimized mosaic id will be the first stored mosaic
					EXPECT_EQ(expected.Balances.begin()->first, actual.Balances.optimizedMosaicId());
				}

				actual.Balances.optimize(expected.Balances.optimizedMosaicId());
			}

			TTraits::AssertEqual(expected, actual);
		}

		template<typename TTraits, typename TAction>
		void AssertCanSaveValueWithMosaics(size_t numMosaics, const RandomSeed& seed, TAction action) {
			// Arrange:
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);

			// - create a random account state
			auto originalAccountState = CreateRandomAccountState(numMosaics, seed);
			TTraits::CoerceToDesiredFormat(originalAccountState);

			// Act:
			TTraits::Serializer::Save(originalAccountState, stream);

			// Assert:
			ASSERT_EQ(TTraits::CalculatePackedSize(originalAccountState), buffer.size());
			action(originalAccountState, buffer);

			// Sanity: no stream flushes
			EXPECT_EQ(0u, stream.numFlushes());
		}

		template<typename TTraits>
		void AssertCanSaveValueWithMosaics(size_t numMosaics) {
			ForEachRandomSeed([numMosaics](const auto& seed) {
				// Act + Assert:
				AssertCanSaveValueWithMosaics<TTraits>(numMosaics, seed, [numMosaics, &seed](
						const auto& originalAccountState,
						const auto& buffer) {
					// Assert:
					auto savedAccountState = TTraits::DeserializeFromBuffer(buffer.data());
					EXPECT_EQ(numMosaics, savedAccountState.Balances.size());
					AssertEqual<TTraits>(seed, originalAccountState, std::move(savedAccountState));
				});
			});
		}
	}

	SERIALIZER_TEST(CanSaveValueWithNoMosaics) {
		AssertCanSaveValueWithMosaics<TTraits>(0);
	}

	SERIALIZER_TEST(CanSaveValueWithSomeMosaics) {
		AssertCanSaveValueWithMosaics<TTraits>(3);
	}

	SERIALIZER_TEST(CanSaveValueWithManyMosaics) {
		AssertCanSaveValueWithMosaics<TTraits>(GetManyMosaicsCount());
	}

	SERIALIZER_TEST(MosaicsAreSavedInSortedOrder) {
		static constexpr auto Num_Mosaics = 128u;
		AssertCanSaveValueWithMosaics<TTraits>(Num_Mosaics, RandomSeed(), [](const auto&, const auto& buffer) {
			auto lastMosaicId = MosaicId();
			auto firstMosaicOffset = TTraits::Mosaic_Header_Offset + sizeof(MosaicHeader);
			const auto* pMosaic = reinterpret_cast<const model::Mosaic*>(buffer.data() + firstMosaicOffset);
			for (auto i = 0u; i < Num_Mosaics; ++i, ++pMosaic) {
				EXPECT_LT(lastMosaicId, pMosaic->MosaicId) << "expected ordering at " << i;

				lastMosaicId = pMosaic->MosaicId;
			}
		});
	}

	// endregion

	// region Load

	namespace {
		template<typename TTraits>
		void AssertCanLoadValueWithMosaics(size_t numMosaics) {
			ForEachRandomSeed([numMosaics](const auto& seed) {
				// Arrange: create a random account state
				auto originalAccountState = CreateRandomAccountState(numMosaics, seed);
				auto buffer = TTraits::CopyToBuffer(originalAccountState);

				// Act: load the account state
				mocks::MockMemoryStream stream(buffer);
				auto loadedAccountState = TTraits::Serializer::Load(stream);

				// Assert:
				EXPECT_EQ(numMosaics, loadedAccountState.Balances.size());
				AssertEqual<TTraits>(seed, originalAccountState, std::move(loadedAccountState));
			});
		}
	}

	SERIALIZER_TEST(CanLoadValueWithNoMosaics) {
		AssertCanLoadValueWithMosaics<TTraits>(0);
	}

	SERIALIZER_TEST(CanLoadValueWithSomeMosaics) {
		AssertCanLoadValueWithMosaics<TTraits>(3);
	}

	SERIALIZER_TEST(CanLoadValueWithManyMosaics) {
		AssertCanLoadValueWithMosaics<TTraits>(GetManyMosaicsCount());
	}

	SERIALIZER_TEST(CannotLoadAccountStateExtendingPastEndOfStream) {
		// Arrange: create a random account state
		auto buffer = TTraits::CopyToBuffer(CreateRandomAccountState(2, RandomSeed()));

		// - size the buffer one byte too small
		buffer.resize(buffer.size() - 1);
		mocks::MockMemoryStream stream(buffer);

		// Act + Assert:
		EXPECT_THROW(TTraits::Serializer::Load(stream), catapult_runtime_error);
	}

	SERIALIZER_TEST(CannotLoadAccountStateWithUnsupportedFormat) {
		// Arrange: create a random account state
		auto buffer = TTraits::CopyToBuffer(CreateRandomAccountState(2, RandomSeed()));

		// - set an unsupported format
		reinterpret_cast<AccountStateHeader&>(buffer[0]).Format = 2;
		mocks::MockMemoryStream stream(buffer);

		// Act + Assert:
		EXPECT_THROW(TTraits::Serializer::Load(stream), catapult_invalid_argument);
	}

	// endregion

	// region Roundtrip

	namespace {
		template<typename TTraits>
		void AssertCanRoundtripValueWithMosaics(size_t numMosaics) {
			ForEachRandomSeed([numMosaics](const auto& seed) {
				// Arrange: create a random account state
				auto originalAccountState = CreateRandomAccountState(numMosaics, seed);
				TTraits::CoerceToDesiredFormat(originalAccountState);

				// Act:
				auto result = test::RunRoundtripBufferTest<typename TTraits::Serializer>(originalAccountState);

				// Assert:
				EXPECT_EQ(numMosaics, result.Balances.size());
				AssertEqual<TTraits>(seed, originalAccountState, std::move(result));
			});
		}
	}

	SERIALIZER_TEST(CanRoundtripValueWithNoMosaics) {
		AssertCanRoundtripValueWithMosaics<TTraits>(0);
	}

	SERIALIZER_TEST(CanRoundtripValueWithSomeMosaics) {
		AssertCanRoundtripValueWithMosaics<TTraits>(3);
	}

	SERIALIZER_TEST(CanRoundtripValueWithManyMosaics) {
		AssertCanRoundtripValueWithMosaics<TTraits>(GetManyMosaicsCount());
	}

	// endregion
}}
