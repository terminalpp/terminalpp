#pragma once

#include <unordered_map>

#include "ui/font.h"

namespace tpp {
    template<typename T>
    class Font {
	public:

		static Font* GetOrCreate(ui::Font font, unsigned height) {
			unsigned id = (height << 8) + helpers::pointer_cast<uint8_t*>(&font)[0];
			auto i = Fonts_.find(id);
			if (i == Fonts_.end())
				i = Fonts_.insert(std::make_pair(id, Create(font, height))).first;
			return i->second;
		}

		/** Doesn't do anything, but is present so that specializations can override it. 
		 */
		~Font() {
		}

		ui::Font font() const {
			return font_;
		}

		unsigned cellWidthPx() const {
			return cellWidthPx_;
		}

		unsigned cellHeightPx() const {
			return cellHeightPx_;
		}

		T const& nativeHandle() const {
			return nativeHandle_;
		}

		float ascent() const {
			return ascent_;
		}

		float underlineOffset() const {
			return underlineOffset_;
		}

		float underlineThickness() const {
			return underlineThickness_;
		}

		float strikethroughOffset() const {
			return strikethroughOffset_;
		}

		float strikethroughThickness() const {
			return strikethroughThickness_;
		}


	private:

		/** This must be implemented by each platform specification. 
		 */
		static Font* Create(ui::Font font, unsigned height);

		static std::unordered_map<unsigned, Font*> Fonts_;
		
		Font(ui::Font font, unsigned cellWidthPx, unsigned cellHeightPx, float ascent, T const& nativeHandle) :
			font_{font},
			cellWidthPx_{cellWidthPx},
			cellHeightPx_{cellHeightPx},
			ascent_{ascent},
			underlineOffset_{0},
			underlineThickness_{1},
			strikethroughOffset_{0},
			strikethroughThickness_{1},
			nativeHandle_{nativeHandle} {
		}

		ui::Font font_;

		unsigned cellWidthPx_;
		unsigned cellHeightPx_;
		float ascent_;
		float underlineOffset_;
		float underlineThickness_;
		float strikethroughOffset_;
		float strikethroughThickness_;

		T nativeHandle_;

	}; // tpp::Font

	template<typename T>
	std::unordered_map<unsigned, Font<T>*> Font<T>::Fonts_;


} // namespace tpp