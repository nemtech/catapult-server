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

#include "Address.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/utils/Base32.h"
#include "catapult/utils/Casting.h"
#include "catapult/exceptions.h"

namespace catapult { namespace model {

	namespace {
		constexpr uint8_t Checksum_Size = 3;
		constexpr size_t Address_Encoded_Size = 39;

		// notice that Base32 implementation only supports decoded data that is a multiple of Base32_Decoded_Block_Size
		// so there is some (ugly) handling of forcing address to/from proper size multiple

		struct PaddedAddress_tag { static constexpr size_t Size = 25; };
		using PaddedAddress = utils::ByteArray<PaddedAddress_tag>;

		std::string PadAddressString(const std::string& str) {
			return str + "A";
		}
	}

	Address StringToAddress(const std::string& str) {
		if (Address_Encoded_Size != str.size())
			CATAPULT_THROW_RUNTIME_ERROR_1("encoded address has wrong size", str.size());

		PaddedAddress paddedAddress;
		utils::Base32Decode(PadAddressString(str), paddedAddress);
		return paddedAddress.copyTo<Address>();
	}

	std::string AddressToString(const Address& address) {
		auto str = utils::Base32Encode(address.copyTo<PaddedAddress>());
		str.pop_back();
		return str;
	}

	std::string PublicKeyToAddressString(const Key& publicKey, NetworkIdentifier networkIdentifier) {
		return AddressToString(PublicKeyToAddress(publicKey, networkIdentifier));
	}

	Address PublicKeyToAddress(const Key& publicKey, NetworkIdentifier networkIdentifier) {
		// step 1: sha3 hash of the public key
		Hash256 publicKeyHash;
		crypto::Sha3_256(publicKey, publicKeyHash);

		// step 2: ripemd160 hash of (1)
		Hash160 step2Hash;
		crypto::Ripemd160(publicKeyHash, step2Hash);

		// step 3: add network identifier byte in front of (2)
		Address decoded;
		decoded[0] = utils::to_underlying_type(networkIdentifier);
		std::memcpy(&decoded[1], &step2Hash[0], Hash160::Size);

		// step 4: concatenate (3) and the checksum of (3)
		Hash256 step3Hash;
		crypto::Sha3_256(RawBuffer{ decoded.data(), Hash160::Size + 1 }, step3Hash);
		std::copy(step3Hash.cbegin(), step3Hash.cbegin() + Checksum_Size, decoded.begin() + Hash160::Size + 1);

		return decoded;
	}

	bool IsValidAddress(const Address& address, NetworkIdentifier networkIdentifier) {
		if (utils::to_underlying_type(networkIdentifier) != address[0])
			return false;

		Hash256 hash;
		auto checksumBegin = Address::Size - Checksum_Size;
		crypto::Sha3_256(RawBuffer{ address.data(), checksumBegin }, hash);

		return std::equal(hash.begin(), hash.begin() + Checksum_Size, address.begin() + checksumBegin);
	}

	namespace {
		bool IsValidEncodedAddress(
				const std::string& encoded,
				const std::function<NetworkIdentifier (uint8_t)>& networkIdentifierAccessor) {
			if (Address_Encoded_Size != encoded.size())
				return false;

			PaddedAddress decoded;
			return utils::TryBase32Decode(PadAddressString(encoded), decoded)
					&& IsValidAddress(decoded.copyTo<Address>(), networkIdentifierAccessor(decoded[0]))
					&& 0 == decoded[PaddedAddress::Size - 1];
		}
	}

	bool IsValidEncodedAddress(const std::string& encoded, NetworkIdentifier networkIdentifier) {
		return IsValidEncodedAddress(encoded, [networkIdentifier](auto) { return networkIdentifier; });
	}

	bool TryParseValue(const std::string& str, Address& parsedValue) {
		if (!IsValidEncodedAddress(str, [](auto firstDecodedByte) { return static_cast<NetworkIdentifier>(firstDecodedByte); }))
			return false;

		parsedValue = StringToAddress(str);
		return true;
	}
}}
