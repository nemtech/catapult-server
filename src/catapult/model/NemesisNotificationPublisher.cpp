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

#include "NemesisNotificationPublisher.h"
#include "BlockChainConfiguration.h"
#include "NotificationSubscriber.h"

namespace catapult { namespace model {

	NemesisNotificationPublisherOptions ExtractNemesisNotificationPublisherOptions(const BlockChainConfiguration& config) {
		NemesisNotificationPublisherOptions options;

		if (0 < config.HarvestNetworkPercentage)
			options.SpecialAccountAddresses.insert(config.HarvestNetworkFeeSinkAddress);

		return options;
	}

	namespace {
		class NemesisNotificationPublisherDecorator : public NotificationPublisher {
		public:
			NemesisNotificationPublisherDecorator(
					std::unique_ptr<const NotificationPublisher>&& pPublisher,
					const NemesisNotificationPublisherOptions& options)
					: m_pPublisher(std::move(pPublisher))
					, m_options(options)
			{}

		public:
			void publish(const WeakEntityInfo& entityInfo, NotificationSubscriber& sub) const override {
				for (const auto& address : m_options.SpecialAccountAddresses)
					sub.notify(AccountAddressNotification(address));

				m_pPublisher->publish(entityInfo, sub);
			}

		private:
			std::unique_ptr<const NotificationPublisher> m_pPublisher;
			const NemesisNotificationPublisherOptions& m_options;
		};
	}

	std::unique_ptr<const NotificationPublisher> CreateNemesisNotificationPublisher(
			std::unique_ptr<const NotificationPublisher>&& pPublisher,
			const NemesisNotificationPublisherOptions& options) {
		return std::make_unique<NemesisNotificationPublisherDecorator>(std::move(pPublisher), options);
	}
}}
