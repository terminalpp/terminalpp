#pragma once

namespace vterm {

	/** Represents a key. 
	
	 */
	class Key {
	public:
		static constexpr unsigned Up = 1024;
		static constexpr unsigned Down = 1025;
		static constexpr unsigned Left = 1026;
		static constexpr unsigned Right = 1027;
		static constexpr unsigned PageUp = 1028;
		static constexpr unsigned PageDown = 1029;
		static constexpr unsigned Home = 1030;
		static constexpr unsigned End = 1031;
		static constexpr unsigned Insert = 1032;
		static constexpr unsigned Delete = 1033;
		static constexpr unsigned Backspace = 1034;
		static constexpr unsigned PrintScreen = 1035;
		static constexpr unsigned Esc = 1036;
		static constexpr unsigned F1 = 1037;
		static constexpr unsigned F2 = 1038;
		static constexpr unsigned F3 = 1039;
		static constexpr unsigned F4 = 1040;
		static constexpr unsigned F5 = 1041;
		static constexpr unsigned F6 = 1042;
		static constexpr unsigned F7 = 1043;
		static constexpr unsigned F8 = 1044;
		static constexpr unsigned F9 = 1045;
		static constexpr unsigned F10 = 1046;
		static constexpr unsigned F11 = 1047;
		static constexpr unsigned F12 = 1048;

		unsigned codepoint() const {
			return codepoint_;
		}

		bool special() const {
			return special_;
		}

		bool shift() const {
			return shift_;
		}

		bool ctrl() const {
			return ctrl_;
		}

		bool alt() const {
			return alt_;
		}

		bool meta() const {
			return meta_;
		}

		Key(unsigned codepoint, bool special = false) :
			codepoint_(codepoint),
			special_(special),
			shift_(0),
			ctrl_(0),
			alt_(0),
			meta_(0) {
		}

	private:
		union {
			struct {
				unsigned codepoint_ : 21;
				unsigned special_ : 1;
				unsigned shift_ : 1;
				unsigned ctrl_ : 1;
				unsigned alt_ : 1;
				unsigned meta_ : 1;
			};
			unsigned raw_;
		};


	};


} // namespace vterm