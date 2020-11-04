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
#include "DelegatePrioritizationPolicy.h"
#include <boost/filesystem/path.hpp>
#include <string>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace harvesting {

	/// Harvesting configuration settings.
	struct HarvestingConfiguration {
	public:
		/// Harvester signing private key. Used only when harvesting is enabled.
		std::string HarvesterSigningPrivateKey;

		/// Harvester VRF private key. Used only when harvesting is enabled.
		std::string HarvesterVrfPrivateKey;

		/// Set to \c true and configure both harvester keys to enable harvesting.
		bool EnableAutoHarvesting;

		/// Maximum number of unlocked accounts, i.e., the maximum number of
		/// delegated harvesting accounts.
		uint32_t MaxUnlockedAccounts;

		/// Prioritization policy used to keep accounts once the maximum number of
		/// delegated harvesting accounts is reached. Possible values are \c Age
		/// and \c Importance.
		harvesting::DelegatePrioritizationPolicy DelegatePrioritizationPolicy;

		/// Address of the account receiving part of the harvested fee.
		Address BeneficiaryAddress;

	private:
		HarvestingConfiguration() = default;

	public:
		/// Creates an uninitialized harvesting configuration.
		static HarvestingConfiguration Uninitialized();

	public:
		/// Loads a harvesting configuration from \a bag.
		static HarvestingConfiguration LoadFromBag(const utils::ConfigurationBag& bag);

		/// Loads a harvesting configuration from \a resourcesPath.
		static HarvestingConfiguration LoadFromPath(const boost::filesystem::path& resourcesPath);
	};
}}
