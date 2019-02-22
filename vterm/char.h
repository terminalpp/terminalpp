#pragma once

namespace vterm {

	/** Unicode character representation. 

	    Characters for the virtual terminal are encoded in UTF-16, but for simplicity reasons each character is 32 bits long, i.e. capable of encoding the entire unicode. 

		TODO technically, UTF-32 might not be the best representation and we can use UTF-8 and recode on windows because Linux and other are AFAIK IMHO UTF8. Not that it matters too much now. 
	 */
	class Char {
	public:

		/** Representation of a character in UTF-16 encoding. 
		 */
		class UTF16 {
		public:
			wchar_t first;
			wchar_t second;

			unsigned size() const {
				NOT_IMPLEMENTED;
			}

			wchar_t const * c_str() const {
				return & first;
			}

		};

		/** Default character is empty space.
		 */
		Char():
			first_(32),
			second_(0) {
		}

		/** Creates char from an ASCII code (0 .. 127). 
		 */
		Char(char from) :
			first_(from),
			second_(0) {
		}

		UTF16 toUTF16() const {
			NOT_IMPLEMENTED;
		}

	private:
		wchar_t first_;
		wchar_t second_;

	};


} // namespace vterm




#ifdef FOO
// TODO perhaps this is nicer representation? 

	/** Character representation in multiple encodings.

		Char is the umbrella class acting as a namespace, defining both the encodings available and the actual representations of characters in these encodings.

		The encodings must support conversion to and from any combination as long as it makes sense.
	 */
class Char2 {
public:

	/** Different character encodings available.

		The following encodings are supported:

		ASCII - the default ASCII character set (0..127)
		UTF8 - full Unicode characters, variable length, UTF8 encoding.
		UTF16 - full Unicode characters, variable length, UTF16 encoding.
		UTF32 - full Unicode codewords, fixed length.
	 */
	enum class Encoding {
		ASCII,
		UTF8,
		UTF16,
		UTF32
	};

	template<Encoding E>
	class Representation {
	};

	typedef Representation<Encoding::ASCII> ASCII;
	typedef Representation<Encoding::UTF8> UTF8;
	typedef Representation<Encoding::UTF16> UTF16;
	typedef Representation<Encoding::UTF32> UTF32;
};

template<>
class Char2::Representation<Char2::Encoding::ASCII> {
public:

private:
	char raw_;
};

template<>
class Char2::Representation<Char2::Encoding::UTF8> {

};

template<>
class Char2::Representation<Char2::Encoding::UTF16> {

};

template<>
class Char2::Representation<Char2::Encoding::UTF32> {

};

#endif