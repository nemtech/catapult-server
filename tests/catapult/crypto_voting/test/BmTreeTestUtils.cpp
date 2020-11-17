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

#include "BmTreeTestUtils.h"

namespace catapult { namespace test {

	void AssertOptions(const crypto::BmOptions& expected, const crypto::BmOptions& actual) {
		EXPECT_EQ(expected.StartKeyIdentifier, actual.StartKeyIdentifier);
		EXPECT_EQ(expected.EndKeyIdentifier, actual.EndKeyIdentifier);
	}

	void AssertZeroedKeys(
			const std::vector<uint8_t>& buffer,
			size_t offset,
			size_t keysCount,
			const std::unordered_set<size_t>& expectedZeroedIndexes,
			const std::string& message) {
		for (auto i = 0u; i < keysCount; ++i) {
			Key key;
			std::memcpy(key.data(), &buffer[offset + i * BmTreeSizes::Level_Entry_Size], Key::Size);

			if (expectedZeroedIndexes.cend() == expectedZeroedIndexes.find(i))
				EXPECT_NE(Key(), key) << message << " key should not be zeroed at " << i;
			else
				EXPECT_EQ(Key(), key) << message << " key should be zeroed at " << i;
		}
	}
}}
