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

#if defined(__GNUC__) && !defined(__clang__)
// gcc raises `redundant-move` when explicit std::move is present
#define PORTABLE_MOVE(X) X
#else
// clang requires explicit std::move to avoid `return-std-move-in-c++11`
#define PORTABLE_MOVE(X) std::move(X)
#endif

#if defined(__APPLE__)
// take extra care to ensure that typeinfos are imported correctly so that all sanitizers pass
#define STRICT_SYMBOL_VISIBILITY 1
#else
#define STRICT_SYMBOL_VISIBILITY 0
#endif

#ifndef MSVC
#define MAY_ALIAS __attribute__((__may_alias__))
#else
#define MAY_ALIAS
#endif
