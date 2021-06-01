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

#include "catapult/local/server/StaticNodeRefreshService.h"
#include "catapult/ionet/PacketSocket.h"
#include "tests/test/core/ThreadPoolTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/local/ServiceTestUtils.h"
#include "tests/test/net/RemoteAcceptServer.h"
#include "tests/test/net/SocketTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace local {

#define TEST_CLASS StaticNodeRefreshServiceTests

	namespace {
		constexpr auto Service_Name = "snr.server_connector";
		constexpr auto Task_Name = "static node refresh task";

		struct StaticNodeRefreshServiceTraits {
			static auto CreateRegistrar(const std::vector<ionet::Node>& staticNodes) {
				return CreateStaticNodeRefreshServiceRegistrar(staticNodes);
			}

			static auto CreateRegistrar() {
				return CreateRegistrar(std::vector<ionet::Node>());
			}
		};

		using TestContext = test::ServiceLocatorTestContext<StaticNodeRefreshServiceTraits>;
	}

	// region basic

	ADD_SERVICE_REGISTRAR_INFO_TEST(StaticNodeRefresh, Initial)

	TEST(TEST_CLASS, CanBootService) {
		// Arrange:
		TestContext context;

		// Act:
		context.boot();

		// Assert:
		EXPECT_EQ(1u, context.locator().numServices());
		EXPECT_EQ(0u, context.locator().counters().size());

		EXPECT_TRUE(!!context.locator().service<void>(Service_Name));
	}

	TEST(TEST_CLASS, CanShutdownService) {
		// Arrange:
		TestContext context;

		// Act:
		context.boot();
		context.shutdown();

		// Assert:
		EXPECT_EQ(1u, context.locator().numServices());
		EXPECT_EQ(0u, context.locator().counters().size());

		EXPECT_FALSE(!!context.locator().service<void>(Service_Name));
	}

	// endregion

	// region task

	namespace {
		ionet::Node CreateNamedLocalHostNode(
				const Key& publicKey,
				const std::string& endpointHost,
				unsigned short portIncrement,
				const std::string& name) {
			return ionet::Node(
					{ publicKey, "" },
					{ endpointHost, static_cast<unsigned short>(test::GetLocalHostPort() + portIncrement) },
					{ model::UniqueNetworkFingerprint(), name });
		}

		class AcceptorServer {
		public:
			AcceptorServer() : m_pPool(test::CreateStartedIoThreadPool(1))
			{}

		public:
			const Key& publicKeyAt(size_t index) {
				return m_servers[index]->caPublicKey();
			}

		public:
			void startAccept() {
				m_acceptors.push_back(std::make_unique<test::TcpAcceptor>(
						m_pPool->ioContext(),
						static_cast<uint16_t>(test::GetLocalHostPort() + m_acceptors.size())));

				m_servers.push_back(std::make_unique<test::RemoteAcceptServer>());
				m_servers.back()->start(*m_acceptors.back(), [](const auto&) {});
			}

		private:
			std::unique_ptr<thread::IoThreadPool> m_pPool;
			std::vector<std::unique_ptr<test::TcpAcceptor>> m_acceptors;
			std::vector<std::unique_ptr<test::RemoteAcceptServer>> m_servers;
		};

		auto SpawnAcceptors(size_t count) {
			auto pAcceptorServer = std::make_unique<AcceptorServer>();
			for (auto i = 0u; i < count; ++i)
				pAcceptorServer->startAccept();

			return pAcceptorServer;
		}
	}

	TEST(TEST_CLASS, TasksAreRegistered) {
		test::AssertRegisteredTasks(TestContext(), { Task_Name });
	}

	TEST(TEST_CLASS, RefreshSucceedsWhenThereAreNoStaticNodes) {
		// Arrange:
		TestContext context;

		auto staticNodes = std::vector<ionet::Node>();
		context.boot(staticNodes);

		const auto& nodes = context.testState().state().nodes();
		test::RunTaskTestPostBoot(context, 1, Task_Name, [&nodes](const auto& task) {
			// Act:
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ(thread::TaskResult::Continue, result);

			auto nodesView = nodes.view();
			EXPECT_EQ(0u, nodesView.size());
		});
	}

	namespace {
		void AssertRefreshSucceedsWhenThereAreMultipleResolvableStaticNodes(const std::string& endpointHost) {
			// Arrange:
			TestContext context;

			// - set up challenge responses
			auto pAcceptorServer = SpawnAcceptors(3);

			// - set up static nodes
			auto staticNodes = std::vector<ionet::Node>{
				CreateNamedLocalHostNode(pAcceptorServer->publicKeyAt(0), endpointHost, 0, "alice"),
				CreateNamedLocalHostNode(pAcceptorServer->publicKeyAt(1), endpointHost, 1, "bob"),
				CreateNamedLocalHostNode(pAcceptorServer->publicKeyAt(2), endpointHost, 2, "charlie")
			};
			context.boot(staticNodes);

			const auto& nodes = context.testState().state().nodes();
			test::RunTaskTestPostBoot(context, 1, Task_Name, [&nodes, &pAcceptorServer, &endpointHost](const auto& task) {
				// Act:
				auto result = task.Callback().get();

				// Assert:
				EXPECT_EQ(thread::TaskResult::Continue, result);

				auto nodesView = nodes.view();
				EXPECT_EQ(3u, nodesView.size());
				auto expectedContents = test::BasicNodeDataContainer{
					{ pAcceptorServer->publicKeyAt(0), "alice", ionet::NodeSource::Static },
					{ pAcceptorServer->publicKeyAt(1), "bob", ionet::NodeSource::Static },
					{ pAcceptorServer->publicKeyAt(2), "charlie", ionet::NodeSource::Static }
				};
				EXPECT_EQ(expectedContents, test::CollectAll(nodesView));

				// - check that hosts are set correctly (identity host is resolved but endpoint host is not)
				nodesView.forEach([&endpointHost](const auto& node, const auto&) {
					EXPECT_EQ("127.0.0.1", node.identity().Host);
					EXPECT_EQ(endpointHost, node.endpoint().Host);
				});
			});
		}
	}

	TEST(TEST_CLASS, RefreshSucceedsWhenThereAreMultipleResolvableStaticNodes_LoopbackAddress) {
		AssertRefreshSucceedsWhenThereAreMultipleResolvableStaticNodes("127.0.0.1");
	}

	TEST(TEST_CLASS, RefreshSkipsInvalidStaticNodes) {
		// Arrange:
		TestContext context;

		// - set up challenge responses
		auto pAcceptorServer = SpawnAcceptors(2);

		// - set up static nodes
		auto staticNodes = std::vector<ionet::Node>{
			CreateNamedLocalHostNode(pAcceptorServer->publicKeyAt(0), "x&y", 0, "alice"), // invalid host
			CreateNamedLocalHostNode(pAcceptorServer->publicKeyAt(1), "127.0.0.1", 1, "bob") // valid host
		};
		context.boot(staticNodes);

		const auto& nodes = context.testState().state().nodes();
		test::RunTaskTestPostBoot(context, 1, Task_Name, [&nodes, &pAcceptorServer](const auto& task) {
			// Act:
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ(thread::TaskResult::Continue, result);

			auto nodesView = nodes.view();
			EXPECT_EQ(1u, nodesView.size());
			auto expectedContents = test::BasicNodeDataContainer{
				{ pAcceptorServer->publicKeyAt(1), "bob", ionet::NodeSource::Static }
			};
			EXPECT_EQ(expectedContents, test::CollectAll(nodesView));

			// - check that hosts are set correctly
			nodesView.forEach([](const auto& node, const auto&) {
				EXPECT_EQ("127.0.0.1", node.identity().Host);
				EXPECT_EQ("127.0.0.1", node.endpoint().Host);
			});
		});
	}

	TEST(TEST_CLASS, RefreshSkipsLocalHost) {
		// Arrange:
		TestContext context;

		// - set up single node representing local node
		auto staticNodes = std::vector<ionet::Node>{
			CreateNamedLocalHostNode(context.publicKey(), "127.0.0.1", 0, "bob")
		};
		context.boot(staticNodes);

		// - set up challenge responses
		auto pAcceptorServer = SpawnAcceptors(1);

		const auto& nodes = context.testState().state().nodes();
		test::RunTaskTestPostBoot(context, 1, Task_Name, [&nodes](const auto& task) {
			// Act:
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ(thread::TaskResult::Continue, result);

			// - request was bypassed because it was local node
			auto nodesView = nodes.view();
			EXPECT_EQ(0u, nodesView.size());
		});
	}

	// endregion
}}
