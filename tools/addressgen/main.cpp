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

#include "tools/ToolMain.h"
#include "tools/AccountTool.h"
#include "tools/ToolThreadUtils.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/SecureRandomGenerator.h"
#include "catapult/model/Address.h"
#include "catapult/model/NetworkIdentifier.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/utils/SpinLock.h"
#include <boost/asio.hpp>
#include <list>
#include <thread>

namespace catapult { namespace tools { namespace addressgen {

	namespace {
		// region KeyCollection

		class KeyCollection {
		public:
			struct CandidateDescriptor {
			public:
				CandidateDescriptor(model::NetworkIdentifier networkIdentifier, crypto::PrivateKey&& privateKey)
						: KeyPair(crypto::KeyPair::FromPrivate(std::move(privateKey)))
						, AddressString(model::AddressToString(model::PublicKeyToAddress(KeyPair.publicKey(), networkIdentifier)))
				{}

			public:
				crypto::KeyPair KeyPair;
				std::string AddressString;
			};

		public:
			bool isComplete() const {
				return std::all_of(m_descriptors.cbegin(), m_descriptors.cend(), IsComplete);
			}

		public:
			void addSearchPattern(const std::string& pattern) {
				SearchDescriptor descriptor;
				descriptor.SearchString = pattern;

				if ('^' == pattern[0]) {
					descriptor.MatchStart = true;
					PopFront(descriptor.SearchString);
				} else if ('$' == pattern.back()) {
					descriptor.MatchEnd = true;
					descriptor.SearchString.pop_back();
				}

				m_descriptors.push_back(std::move(descriptor));
			}

			const crypto::KeyPair* accept(CandidateDescriptor&& candidateDescriptor) {
				const auto& addressString = candidateDescriptor.AddressString;

				for (auto& descriptor : m_descriptors) {
					auto searchString = descriptor.SearchString;
					while (!searchString.empty() || (searchString.size() > descriptor.BestMatchSize || !descriptor.pBestKeyPair)) {
						auto matchIndex = addressString.find(searchString);
						auto isMatch = std::string::npos != matchIndex;
						if (descriptor.MatchStart)
							isMatch = 0 == matchIndex;
						else if (descriptor.MatchEnd)
							isMatch = matchIndex + searchString.size() == addressString.size();

						if (isMatch) {
							if (!searchString.empty()) {
								CATAPULT_LOG(info)
										<< "searching for '" << descriptor.SearchString << "' found " << addressString
										<< " (" << searchString.size() << "/" << descriptor.SearchString.size() << ")";
							}

							descriptor.BestMatchSize = searchString.size();
							descriptor.pBestKeyPair = std::make_unique<crypto::KeyPair>(std::move(candidateDescriptor.KeyPair));

							if (IsComplete(descriptor))
								return descriptor.pBestKeyPair.get();
						}

						if (descriptor.MatchEnd)
							PopFront(searchString);
						else
							searchString.pop_back();
					}
				}

				return nullptr;
			}

		private:
			static void PopFront(std::string& str) {
				str = str.substr(1);
			}

		private:
			struct SearchDescriptor {
				std::string SearchString;

				bool MatchStart = false;
				bool MatchEnd = false;
				size_t BestMatchSize = 0;

				std::unique_ptr<crypto::KeyPair> pBestKeyPair;
			};

			static bool IsComplete(const SearchDescriptor& descriptor) {
				return descriptor.SearchString.size() == descriptor.BestMatchSize && !!descriptor.pBestKeyPair;
			}

		private:
			std::list<SearchDescriptor> m_descriptors;
		};

		// endregion

		// region AddressGeneratorTool

		class AddressGeneratorTool : public AccountTool {
		public:
			AddressGeneratorTool() : AccountTool("Address Generator Tool", AccountTool::InputDisposition::Optional)
			{}

		private:
			void prepareAdditionalOptions(OptionsBuilder& optionsBuilder) override {
				optionsBuilder("count,c",
						OptionsValue<uint32_t>()->default_value(1),
						"number of random keys to generate for each pattern");
				optionsBuilder("threads,t",
						OptionsValue<uint32_t>()->default_value(2 * std::thread::hardware_concurrency()),
						"number of threads to use");
			}

			void process(const Options& options, const std::vector<std::string>& values, AccountPrinter& printer) override {
				auto count = options["count"].as<uint32_t>();

				// network value is validated before process is called
				model::NetworkIdentifier networkIdentifier;
				model::TryParseValue(options["network"].as<std::string>(), networkIdentifier);

				KeyCollection keys;
				for (auto i = 0u; i < count; ++i) {
					for (const auto& value : values)
						keys.addSearchPattern(value);
				}

				utils::SpinLock keysLock;
				auto pPool = CreateStartedThreadPool(options["threads"].as<uint32_t>());
				for (auto i = 0u; i < pPool->numWorkerThreads(); ++i) {
					pPool->ioContext().dispatch([networkIdentifier, &printer, &keys, &keysLock]() {
						auto randomGenerator = crypto::SecureRandomGenerator();

						for (;;) {
							Key privateKeyBuffer;
							randomGenerator.fill(privateKeyBuffer.data(), privateKeyBuffer.size());
							auto privateKey = crypto::PrivateKey::FromBufferSecure(privateKeyBuffer);
							auto candidateDescriptor = KeyCollection::CandidateDescriptor(networkIdentifier, std::move(privateKey));

							utils::SpinLockGuard guard(keysLock);
							const auto* pNewKeyPair = keys.accept(std::move(candidateDescriptor));
							if (pNewKeyPair)
								printer.print(*pNewKeyPair);

							if (keys.isComplete())
								return;
						}
					});
				}

				pPool->join();
			}
		};

		// endregion
	}
}}}

int main(int argc, const char** argv) {
	catapult::tools::addressgen::AddressGeneratorTool tool;
	return catapult::tools::ToolMain(argc, argv, tool);
}
