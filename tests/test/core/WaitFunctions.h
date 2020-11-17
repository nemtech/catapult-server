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
#include "catapult/functions.h"
#include <stdint.h>

namespace boost { namespace asio { class io_context; } }

namespace catapult { namespace test {

	/// Predicate that returns \c true if a function should continue waiting.
	using ShouldWaitPredicate = predicate<>;

	/// Wait function.
	using WaitFunction = consumer<boost::asio::io_context&, const ShouldWaitPredicate&>;

	/// Creates a synchronous wait function that waits for intervals of \a waitMillis ms.
	WaitFunction CreateSyncWaitFunction(uint32_t waitMillis);

	/// Creates an asynchronous wait function that waits for intervals of \a waitMillis ms.
	WaitFunction CreateAsyncWaitFunction(uint32_t waitMillis);
}}
