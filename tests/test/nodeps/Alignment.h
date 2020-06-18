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
#include "catapult/types.h"

namespace catapult { namespace test {

	/// Gets the desired alignment for type T.
	template<typename T>
	constexpr size_t GetRequiredAlignment() {
		if constexpr (utils::traits::is_scalar_v<T>)
			return sizeof(T); // require scalars to be aligned on their natural boundaries
		else
			return sizeof(uint64_t); // require non-scalars to be aligned on 64-bit boundaries
	}
}}

/// Asserts that \a FIELD has proper alignment within \a STRUCT.
#define EXPECT_ALIGNED_(STRUCT, FIELD) EXPECT_EQ(0u, offsetof(STRUCT, FIELD) % test::GetRequiredAlignment<decltype(STRUCT::FIELD)>())

#ifndef _MSC_VER
#define EXPECT_ALIGNED_WITH_PRAGMAS(STRUCT, FIELD) \
	do { \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Winvalid-offsetof\"") /* allow offsetof on non-standard layout type */ \
		EXPECT_ALIGNED_(STRUCT, FIELD); \
		_Pragma("GCC diagnostic pop") \
	} while (false)

// extra indirection is needed for GCC Release builds
#define EXPECT_ALIGNED EXPECT_ALIGNED_WITH_PRAGMAS
#else
#define EXPECT_ALIGNED EXPECT_ALIGNED_
#endif
