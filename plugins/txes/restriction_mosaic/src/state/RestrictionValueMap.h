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
#include <algorithm>
#include <set>
#include <vector>
#include <stdint.h>

namespace catapult { namespace state {

	/// Map of restriction values.
	template<typename TValue>
	class RestrictionValueMap {
	public:
		/// Gets the number of values in the map.
		size_t size() const {
			return m_keyValuePairs.size();
		}

		/// Gets all restriction keys.
		std::set<uint64_t> keys() const {
			std::set<uint64_t> keys;
			for (const auto& pair : m_keyValuePairs)
				keys.insert(pair.first);

			return keys;
		}

	private:
		template<typename TKeyValuePairs>
		static auto Find(TKeyValuePairs& keyValuePairs, uint64_t key) {
			return std::find_if(keyValuePairs.begin(), keyValuePairs.end(), [key](const auto& pair) {
				return key == pair.first;
			});
		}

	public:
		/// Tries to get the \a value associated with \a key.
		bool tryGet(uint64_t key, TValue& value) const {
			auto iter = Find(m_keyValuePairs, key);
			if (m_keyValuePairs.cend() == iter)
				return false;

			value = iter->second;
			return true;
		}

		/// Sets the \a value associated with \a key.
		void set(uint64_t key, const TValue& value) {
			auto iter = Find(m_keyValuePairs, key);
			if (m_keyValuePairs.cend() != iter)
				iter->second = value;
			else
				m_keyValuePairs.emplace_back(key, value);
		}

		/// Removes the value associated with \a key.
		void remove(uint64_t key) {
			auto iter = Find(m_keyValuePairs, key);
			if (m_keyValuePairs.cend() != iter)
				m_keyValuePairs.erase(iter);
		}

	private:
		std::vector<std::pair<uint64_t, TValue>> m_keyValuePairs;
	};
}}
