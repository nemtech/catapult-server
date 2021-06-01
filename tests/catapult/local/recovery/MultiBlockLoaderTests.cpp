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

#include "catapult/local/recovery/MultiBlockLoader.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/NemesisBlockLoader.h"
#include "catapult/subscribers/StateChangeInfo.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/core/mocks/MockMemoryBlockStorage.h"
#include "tests/test/local/BlockStateHash.h"
#include "tests/test/local/FilechainTestUtils.h"
#include "tests/test/local/LocalNodeTestState.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nemesis/NemesisCompatibleConfiguration.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/other/mocks/MockBlockHeightCapturingNotificationObserver.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/TestHarness.h"

namespace catapult { namespace local {

#define TEST_CLASS MultiBlockLoaderTests

	// region CreateBlockDependentNotificationObserverFactory

	namespace {
		enum class ObserverFactoryResult { Transient, Permanent, Unknown };

		ObserverFactoryResult RunObserverFactoryInflectionPointTest(
				const model::Block& lastBlock,
				Height currentBlockHeight,
				Timestamp currentBlockTime) {
			// Arrange:
			auto pCurrentBlock = test::GenerateBlockWithTransactions(0, currentBlockHeight, currentBlockTime);

			// - create configuration
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(2);
			config.MaxDifficultyBlocks = 100;
			config.MaxTransactionLifetime = utils::TimeSpan::FromSeconds(utils::TimeSpan::FromHours(1).seconds() + 44);

			// Act:
			auto observerFactory = CreateBlockDependentNotificationObserverFactory(
					lastBlock,
					config,
					[]() { return std::make_unique<mocks::MockNotificationObserver>("transient"); },
					[]() { return std::make_unique<mocks::MockNotificationObserver>("permanent"); });
			auto pObserver = observerFactory(*pCurrentBlock);

			// Assert:
			return "transient" == pObserver->name()
					? ObserverFactoryResult::Transient
					: "permanent" == pObserver->name() ? ObserverFactoryResult::Permanent : ObserverFactoryResult::Unknown;
		}
	}

	TEST(TEST_CLASS, ObserverFactoryRespectsHeightInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(1234), Timestamp(utils::TimeSpan::FromHours(2).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto height) {
			return RunObserverFactoryInflectionPointTest(lastBlock, height, Timestamp(1));
		};

		// Act + Assert: inflection point is `LastBlockHeight - MaxDifficultyBlocks + 1` [1234 - 100 + 1]
		auto inflectionHeight = Height(1234 - 100 + 1);
		EXPECT_EQ(ObserverFactoryResult::Permanent, runTest(inflectionHeight - Height(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionHeight));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionHeight + Height(1)));
	}

	TEST(TEST_CLASS, ObserverFactoryReturnsTransientObserverWhenThereIsNoHeightInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(50), Timestamp(utils::TimeSpan::FromHours(2).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto height) {
			return RunObserverFactoryInflectionPointTest(lastBlock, height, Timestamp(1));
		};

		// Act + Assert: there is no height inflection point because `LastBlockHeight < MaxDifficultyBlocks` [50 < 100]
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Height(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Height(2)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Height(50)));
	}

	TEST(TEST_CLASS, ObserverFactoryRespectsTimeInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(1234), Timestamp(utils::TimeSpan::FromHours(2).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto time) {
			return RunObserverFactoryInflectionPointTest(lastBlock, Height(1), time);
		};

		// Act + Assert: inflection point is `LastBlockTime - TransactionCacheDuration` [2H - (1H + 22 * 2s)]
		auto inflectionTime = Timestamp(utils::TimeSpan::FromHours(1).millis() - 22 * utils::TimeSpan::FromSeconds(2).millis());
		EXPECT_EQ(ObserverFactoryResult::Permanent, runTest(inflectionTime - Timestamp(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionTime));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionTime + Timestamp(1)));
	}

	TEST(TEST_CLASS, ObserverFactoryReturnsTransientObserverWhenThereIsNoTimeInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(1234), Timestamp(utils::TimeSpan::FromHours(1).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto time) {
			return RunObserverFactoryInflectionPointTest(lastBlock, Height(1), time);
		};

		// Act + Assert: there is no time inflection point because `LastBlockTime < TransactionCacheDuration` [1H < (1H + 22 * 2s)]
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Timestamp(0)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Timestamp(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Timestamp(utils::TimeSpan::FromHours(1).millis())));
	}

	// endregion

	// region LoadBlockChain

	namespace {
		void AddXorResolvers(plugins::PluginManager& pluginManager) {
			pluginManager.addMosaicResolver([](const auto&, const auto& unresolved, auto& resolved) {
				resolved = test::CreateResolverContextXor().resolve(unresolved);
				return true;
			});
			pluginManager.addAddressResolver([](const auto&, const auto& unresolved, auto& resolved) {
				resolved = test::CreateResolverContextXor().resolve(unresolved);
				return true;
			});
		}

		class LoadBlockChainTestContext {
		public:
			LoadBlockChainTestContext() : m_pluginManager(test::CreatePluginManager()) {
				AddXorResolvers(m_pluginManager);
			}

		public:
			const auto& observerBlockHeights() const {
				return m_observerBlockHeights;
			}

			const auto& factoryHeights() const {
				return m_factoryHeights;
			}

			const auto& statusHeights() const {
				return m_statusHeights;
			}

		public:
			void setStorageChainHeight(Height chainHeight) {
				auto storage = m_state.ref().Storage.modifier();

				for (auto height = Height(2); height <= chainHeight; height = height + Height(1)) {
					auto pBlock = test::GenerateBlockWithTransactions(0, height, Timestamp(height.unwrap() * 3000));
					pBlock->Difficulty = Difficulty(Difficulty().unwrap() + height.unwrap());
					storage.saveBlock(test::BlockToBlockElement(*pBlock));
				}

				storage.commit();
			}

			model::ChainScore load(Height startHeight) {
				auto observerFactory = [this](const auto& block) {
					this->m_factoryHeights.push_back(block.Height);
					return std::make_unique<mocks::MockBlockHeightCapturingNotificationObserver>(this->m_observerBlockHeights);
				};

				auto score = LoadBlockChain(observerFactory, m_pluginManager, m_state.ref(), startHeight, [this](const auto& status) {
					// check status for internal consistency
					auto newComputedScore = m_statusScore;
					newComputedScore += status.StateChangeInfo.ScoreDelta;

					EXPECT_EQ(status.BlockElement.Block.Height, status.StateChangeInfo.Height);
					EXPECT_EQ(newComputedScore, status.ChainScore);

					m_statusScore = newComputedScore;
					m_statusHeights.push_back(status.BlockElement.Block.Height);
				});

				// check score for consistency
				EXPECT_EQ(m_statusScore, score);
				return score;
			}

		private:
			std::vector<Height> m_factoryHeights;
			std::vector<Height> m_observerBlockHeights;
			test::LocalNodeTestState m_state;
			plugins::PluginManager m_pluginManager;

			model::ChainScore m_statusScore;
			std::vector<Height> m_statusHeights;
		};

		constexpr uint64_t CalculateExpectedScore(size_t height) {
			// - nemesis difficulty is 0 and nemesis time is 0
			// - all other blocks have a difficulty of base + height
			// - blocks at heights 1 and 2 have time difference of 6s
			// - all other blocks have a time difference of 3s
			return
					Difficulty().unwrap() * (height - 1) // sum base difficulties
					+ height * (height + 1) / 2 // sum difficulty deltas (1..N)
					- 1 // adjust for range (2..N), first block has height 2
					- (6 + (height - 2) * 3); // subtract the time differences
		}
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsZeroBlocksWhenStorageHeightIsOne) {
		// Arrange:
		LoadBlockChainTestContext context;

		// Act:
		auto score = context.load(Height(2));

		// Assert:
		EXPECT_EQ(model::ChainScore(), score);
		EXPECT_EQ(0u, context.observerBlockHeights().size());
		EXPECT_EQ(0u, context.factoryHeights().size());
		EXPECT_EQ(0u, context.statusHeights().size());
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsSingleBlockWhenStorageHeightIsTwo) {
		// Arrange:
		LoadBlockChainTestContext context;
		context.setStorageChainHeight(Height(2));

		// Act:
		auto score = context.load(Height(2));

		// Assert:
		auto expectedHeights = std::vector<Height>{ Height(2) };
		EXPECT_EQ(model::ChainScore(CalculateExpectedScore(2)), score);
		EXPECT_EQ(1u, context.observerBlockHeights().size());
		EXPECT_EQ(expectedHeights, context.observerBlockHeights());
		EXPECT_EQ(expectedHeights, context.factoryHeights());
		EXPECT_EQ(expectedHeights, context.statusHeights());
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsMultipleBlocksWhenStorageHeightIsGreaterThanTwo) {
		// Arrange:
		LoadBlockChainTestContext context;
		context.setStorageChainHeight(Height(7));

		// Act:
		auto score = context.load(Height(2));

		// Assert:
		auto expectedHeights = std::vector<Height>{ Height(2), Height(3), Height(4), Height(5), Height(6), Height(7) };
		EXPECT_EQ(model::ChainScore(CalculateExpectedScore(7)), score);
		EXPECT_EQ(6u, context.observerBlockHeights().size());
		EXPECT_EQ(expectedHeights, context.observerBlockHeights());
		EXPECT_EQ(expectedHeights, context.factoryHeights());
		EXPECT_EQ(expectedHeights, context.statusHeights());
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsMultipleBlocksStartingAtArbitraryHeight) {
		// Arrange: create a storage with 7 blocks
		LoadBlockChainTestContext context;
		context.setStorageChainHeight(Height(7));

		// Act: load blocks 4-7
		auto score = context.load(Height(4));

		// Assert:
		auto expectedHeights = std::vector<Height>{ Height(4), Height(5), Height(6), Height(7) };
		EXPECT_EQ(model::ChainScore(CalculateExpectedScore(7) - CalculateExpectedScore(3)), score);
		EXPECT_EQ(4u, context.observerBlockHeights().size());
		EXPECT_EQ(expectedHeights, context.observerBlockHeights());
		EXPECT_EQ(expectedHeights, context.factoryHeights());
		EXPECT_EQ(expectedHeights, context.statusHeights());
	}

	// endregion

	// region LoadBlockChain - state enabled

	namespace {
		std::vector<Address> GenerateDeterministicAddresses(size_t count) {
			std::vector<Address> addresses;
			for (auto i = 0u; i < count; ++i)
				addresses.push_back(Address{ { static_cast<uint8_t>(1 + i) } });

			return addresses;
		}

		auto CreateBlocks(size_t maxHeight) {
			// each block has at most 20 txes
			const auto numRecipientAccounts = (maxHeight - 1) * 20;

			std::mt19937_64 rnd(0x11223344'55667788ull);
			auto nemesisKeyPairs = test::GetNemesisKeyPairs();
			auto recipients = GenerateDeterministicAddresses(numRecipientAccounts);

			size_t recipientIndex = 0;
			std::vector<std::unique_ptr<model::Block>> blocks;
			for (auto height = 2u; height <= maxHeight; ++height)
				blocks.push_back(test::CreateBlock(nemesisKeyPairs, recipients[recipientIndex++], rnd, height).pBlock);

			return blocks;
		}

		void ExecuteNemesis(const extensions::LocalNodeStateRef& stateRef, const plugins::PluginManager& pluginManager) {
			auto cacheDelta = stateRef.Cache.createDelta();
			extensions::NemesisBlockLoader loader(cacheDelta, pluginManager, pluginManager.createObserver());

			loader.executeAndCommit(stateRef, extensions::StateHashVerification::Disabled);
		}

		void SetBlockChainConfiguration(model::BlockChainConfiguration& config) {
			config.Plugins.emplace("catapult.plugins.hashcache", utils::ConfigurationBag({ { "", { {} } } }));

			// set the number of rollback blocks to zero to avoid unnecessarily influencing height-dominant tests
			config.MaxRollbackBlocks = 0;
		}

		config::CatapultConfiguration CreateStateHashEnabledCatapultConfiguration(const std::string& dataDirectory) {
			auto config = test::CreateCatapultConfigurationWithNemesisPluginExtensions(dataDirectory);
			SetBlockChainConfiguration(const_cast<model::BlockChainConfiguration&>(config.BlockChain));

			const_cast<config::NodeConfiguration&>(config.Node).EnableCacheDatabaseStorage = true;
			const_cast<model::BlockChainConfiguration&>(config.BlockChain).EnableVerifiableState = true;
			return config;
		}

		template<typename TAction>
		void ExecuteWithStorage(io::BlockStorageCache& storage, TAction action) {
			// Arrange:
			test::TempDirectoryGuard tempDataDirectory;
			config::CatapultDataDirectoryPreparer::Prepare(tempDataDirectory.name());

			auto config = CreateStateHashEnabledCatapultConfiguration(tempDataDirectory.name());
			auto pPluginManager = test::CreatePluginManagerWithRealPlugins(config);
			auto observerFactory = [&pluginManager = *pPluginManager](const auto&) { return pluginManager.createObserver(); };

			auto blockChainConfig = pPluginManager->config();
			auto localNodeConfig = test::CreatePrototypicalCatapultConfiguration(std::move(blockChainConfig), tempDataDirectory.name());

			auto cache = pPluginManager->createCache();
			extensions::LocalNodeChainScore score;
			extensions::LocalNodeStateRef stateRef(localNodeConfig, cache, storage, score);
			ExecuteNemesis(stateRef, *pPluginManager);

			// Act:
			LoadBlockChain(observerFactory, *pPluginManager, stateRef, Height(2));

			action(stateRef.Cache, *pPluginManager);
		}

		void RunLoadBlockChainTest(io::BlockStorageCache& storage, size_t maxHeight) {
			// Arrange: create one additional block to simplify test, blocks[0].height = 2
			auto blocks = CreateBlocks(maxHeight + 1);

			// - calculate expected state hash after loading first two blocks (1, 2)
			Hash256 expectedHash;
			ExecuteWithStorage(storage, [&expectedHash, &block = *blocks[0] ](auto& cache, const auto& pluginManager) {
				auto cacheDetachableDelta = cache.createDetachableDelta();
				auto cacheDetachedDelta = cacheDetachableDelta.detach();
				auto pCacheDelta = cacheDetachedDelta.tryLock();

				expectedHash = test::CalculateBlockStateHash(block, *pCacheDelta, pluginManager);
			});

			// Act:
			// - add single block to the storage
			// - compare current state hash with expected hash
			// - calculate next expected hash by using current cache state and next block
			for (auto height = 2u; height <= maxHeight; ++height) {
				const auto& block = *blocks[height - 2];
				{
					auto storageModifier = storage.modifier();
					storageModifier.saveBlock(test::BlockToBlockElement(block));
					storageModifier.commit();
				}

				// - load whole chain and verify hash
				const auto& nextBlock = *blocks[height - 1];
				ExecuteWithStorage(storage, [&expectedHash, &nextBlock](auto& cache, const auto& pluginManager) {
					// Assert:
					// - retrieve state hash calculated when loading chain
					auto hashInfo = cache.createView().calculateStateHash();

					EXPECT_EQ(expectedHash, hashInfo.StateHash);

					// - calculate next expected hash
					auto cacheDetachableDelta = cache.createDetachableDelta();
					auto cacheDetachedDelta = cacheDetachableDelta.detach();
					auto pCacheDelta = cacheDetachedDelta.tryLock();

					expectedHash = test::CalculateBlockStateHash(nextBlock, *pCacheDelta, pluginManager);
				});
			}
		}
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsMultipleBlocks_StateHashEnabled) {
		// Arrange:
		io::BlockStorageCache storage(
				std::make_unique<mocks::MockMemoryBlockStorage>(),
				std::make_unique<mocks::MockMemoryBlockStorage>());

		// Act + Assert:
		RunLoadBlockChainTest(storage, 7);
	}

	// endregion
}}
