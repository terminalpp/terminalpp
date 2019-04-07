#pragma once

namespace vterm {

	/** Represents a key. 
	
	 */
	class Key {
	public:
		static constexpr unsigned None = 0;
		static constexpr unsigned Enter = 13;
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

		Key withShift() const {
			return Key(codepoint_, special_, true, ctrl_, alt_, meta_);
		}

		Key withCtrl() const {
			return Key(codepoint_, special_, shift_, true, alt_, meta_);
		}

		Key withAlt() const {
			return Key(codepoint_, special_, shift_, ctrl_, true, meta_);
		}

		Key withMeta() const {
			return Key(codepoint_, special_, shift_, ctrl_, alt_, true);
		}


		Key(unsigned codepoint, bool special = false, bool shift = false, bool ctrl = false, bool alt = false, bool meta = false) :
			raw_(0),
			codepoint_(codepoint),
			special_(special),
			shift_(shift),
			ctrl_(ctrl),
			alt_(alt),
			meta_(meta) {
		}

		bool operator == (Key const& other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Key const& other) const {
			return raw_ != other.raw_;
		}

	private:
		union {
			unsigned raw_;
			struct {
				unsigned codepoint_ : 21;
				unsigned special_ : 1;
				unsigned shift_ : 1;
				unsigned ctrl_ : 1;
				unsigned alt_ : 1;
				unsigned meta_ : 1;
			};
		};


	};


} // namespace vterm