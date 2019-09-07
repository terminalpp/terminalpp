#pragma once

#include <unordered_map>

#include "ui/font.h"

namespace tpp {

    template<typename T>
    class Font {
	public:
		/** Creates a font to fit the given cell dimensions. 
		 */
	    static T * GetOrCreate(ui::Font font, unsigned cellWidth, unsigned cellHeight) {
			unsigned id = (cellHeight << 8) + helpers::pointer_cast<uint8_t*>(&font)[0];
			auto i = Fonts_.find(id);
			if (i == Fonts_.end())
			    i = Fonts_.insert(std::make_pair(id, new T(font, cellWidth, cellHeight))).first;
			return i->second;
		}

		/** Deletes the fallback cache.
		 */
		virtual ~Font() {
            for (T * f : fallbackCache_)
                delete f;
		}

        virtual bool supportsCodepoint(char32_t codepoint) = 0;

		/** Returns the fallback font that can be used for the given UTF codepoint. 
		 */
		T * fallbackFor(unsigned cellWidth, unsigned cellHeight, char32_t codepoint) {
            for (auto i : fallbackCache_)
                if (i->supportsCodepoint(codepoint))
                    return i;
            T * f = new T(*dynamic_cast<T const *>(this), cellWidth, cellHeight, codepoint);
            fallbackCache_.push_back(f);
            return f;
        }

		ui::Font font() const {
			return font_;
		}

		unsigned widthPx() const {
			return widthPx_;
		}

		unsigned heightPx() const {
			return heightPx_;
		}

		unsigned offsetLeft() const {
			return offsetLeft_;
		}

		unsigned offsetTop() const {
			return offsetTop_;
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

	protected:
		
		static std::unordered_map<unsigned, T*> Fonts_;

        std::vector<T*> fallbackCache_;
		
		Font(ui::Font font) :
			font_{font},
			widthPx_{0},
			heightPx_{0},
			offsetLeft_{0},
			offsetTop_{0},
			ascent_{0},
			underlineOffset_{0},
			underlineThickness_{1},
			strikethroughOffset_{0},
			strikethroughThickness_{1} {
		}

		ui::Font font_;
		unsigned widthPx_;
		unsigned heightPx_;
		unsigned offsetLeft_;
		unsigned offsetTop_;
		float ascent_;
		float underlineOffset_;
		float underlineThickness_;
		float strikethroughOffset_;
		float strikethroughThickness_;

	}; // tpp::Font

	template<typename T>
	std::unordered_map<unsigned, T*> Font<T>::Fonts_;


} // namespace tpp