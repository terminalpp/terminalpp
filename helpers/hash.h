#pragma once

#include "helpers.h"
#include "strings.h"

namespace helpers {


	/** A memory efficient representation of hashes. 

	    The hash is represented as an array of bytes, which halves the memory footprint compared to simple storing the hash in a string as its hexadecimal value. Conversion functions from and to string representation are available. 
	 */
	template<unsigned BYTES>
	class Hash {
	public:
		/** Creates an empty hash initialized to zeroes. 
		 */
		Hash() {
			memset(bytes_, 0, BYTES);
		}

		/** Creates the hash from a hexadecimal string. 

		    The size of the string must correspond to the desired hash size (i.e. be twice as large). 
		 */
		explicit Hash(std::string const & from) {
			fromString(from);
		}

		/** Copy constructor, simple memcpy's the byte array. 
		 */
		Hash(Hash const & from) {
			memcpy(&buffer_, &from.buffer_, BYTES);
		}

		/** Size of the hash in bytes. 
		 */
		size_t rawSize() const {
			return BYTES;
		}

		/** Size of the hexadecimal string representation of the hash in bytes (2 * hash size) 
		 */
		size_t strSize() const {
			return BYTES * 2;
		}

		/** Sets the hash to given hash. 
		 */
		Hash & operator = (Hash & from) {
			memcpy(&buffer_, &from.buffer_, BYTES);
			return *this;
		}

		/** Sets the hash to given string representing hexadecimal value of the appropriate length.
		 */
		Hash & operator = (std::string const & from) {
			fromString(from);
			return *this;
		}

		/** Compares two hashes. 
		 */
		bool operator == (Hash const & other) const {
			return memcmp(&bytes_, &other.bytes_, BYTES * 2) == 0;
		}

		/** Compares two hashes.
		 */
		bool operator != (Hash const & other) const {
			return memcmp(&bytes_, &other.bytes_, BYTES * 2) != 0;
		}

		/** Returns a pointer to the hash internal array. 
		 */
		unsigned char const * raw() const {
			return bytes_;
		}

	private:

		/** Sends the hash as a hexadecimal string to the given output stream. 
		 */
		friend std::ostream & operator << (std::ostream & s, Hash const & h) {
			for (size_t i = 0; i < BYTES; ++i) {
				unsigned x = (h.bytes_[i] >> 4);
				if (x > 9)
					s << 'a' + (x - 10);
				else
					s << '0' + x;
				x = h.bytes_[i] & 0x0f;
				if (x > 9)
					s << 'a' + (x - 10);
				else
					s << '0' + x;
			}
			return s;
		}

		/** Fills the hash from given string of appropriate size. 
		 */
		void fromString(std::string const & from) {
			ASSERT(from.size() == 2 * BYTES) << "Invalid string size " << from.size() << " for hash of size " << BYTES << " (expected string size " << 2 * BYTES << ")";
			for (size_t i = 0; i < BYTES; ++i)
				bytes_[i] = (HexCharToNumber(from[i * 2]) << 8) + HexCharToNumber(from[i * 2 + 1]);
		}

		/** Array storing the hash value. 
		 */
		unsigned char bytes_[BYTES];
	};

	/** MD5 hash, which is 128 bits (16 bytes) long. 
	 */
	typedef Hash<16> HashMD5;

	/** SHA1 hash, which is 160 (20 bytes) long. 
	 */
	typedef Hash<20> HashSHA1;

} // namespace helpers

namespace std {

	/** Template specialization of the std::hash for the helpers::Hash class so that hashes can be used as keys in maps and so on. 
	 */
	template<unsigned BYTES>
	struct hash<helpers::Hash<BYTES>> {
		size_t operator() (helpers::Hash<BYTES> & h) {
			size_t result;
			unsigned i = 0;
			hash<unsigned> hu{};
			while (i + sizeof(unsigned) <= BYTES) {
				result += hu(*reinterpret_cast<unsigned>(h.raw() + i));
				i += sizeof(unsigned);
			}
			hash<unsigned char> hc{};
			while (i != BYTES) {
				result += hc(*(h.raw() + i));
				++i;
			}
			return result;
		}

	};

} // namespace std
