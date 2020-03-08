#pragma once

#include <unordered_map>

#include "helpers/helpers.h"
#include "helpers/char.h"

#include "ui/font.h"
#include "ui2/geometry.h"

namespace tpp2 {

    /** Rendering font information. 
     */
    class FontMetrics {
    public:

        /** Returns the font associated with the font specification. 
         */
        ui2::Font font() const {
            return font_;
        }

        int cellWidth() const {
            return cellWidth_;
        }

        int cellHeight() const {
            return cellHeight_;
        }

        // TODO offset should be point
        int offsetTop() const {
            return offsetTop_;
        }

        int offsetLeft() const {
            return offsetLeft_;
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

        virtual ~FontMetrics() {
            // subclasses should free their native resources here
        }

        static ui2::Font Strip(ui2::Font f) {
            return f.setUnderline(false).setStrikethrough(false).setBlink(false);
        }

    protected:

        FontMetrics(ui2::Font const & font, int cellHeight, int cellWidth = 0):
            font_{Strip(font)},
            cellWidth_{cellWidth},
            cellHeight_{cellHeight},
            offsetLeft_{0},
            offsetTop_{0} {
        }

        ui2::Font font_;

        int cellWidth_;
        int cellHeight_;
        // offset of the character in the cell if the characters need to be centered (for fallback fonts)
        int offsetLeft_;
        int offsetTop_;

        float ascent_;
        float underlineOffset_;
        float underlineThickness_;
        float strikethroughOffset_;
        float strikethroughThickness_;

    }; // tpp::FontMetrics

    template<typename T>
    class Font : public FontMetrics {
    public:
        static T * Get(ui2::Font font, int cellHeight, int cellWidth = 0) {
            size_t id = CreateIdFrom(font, cellHeight);
            auto i = Fonts_.find(id);
            if (i == Fonts_.end()) {
                T * f = new T(font, cellHeight, cellWidth);
                i = Fonts_.insert(std::make_pair(id, f)).first;
            }
            return i->second;
        }

        T * fallbackFor(char32_t codepoint) {
			std::vector<T*> & fallbackCache = FallbackFonts_[CreateIdFrom(font_, cellHeight_)];
            for (auto i : fallbackCache) {
				if (i != this && i->supportsCodepoint(codepoint))
                    return i;
			}
			// if the character we search the fallback for is double width increase the cell width now
			//cellWidth *= helpers::Char::ColumnWidth(codepoint);
            T * f = new T(*static_cast<T const *>(this), codepoint);
            fallbackCache.push_back(f);
            return f;
        }

    protected:

        Font(ui2::Font const & font, int cellHeight, int cellWidth):
            FontMetrics(font, cellHeight, cellWidth) {
        }

        static size_t CreateIdFrom(ui2::Font font, int cellHeight) {
            ui2::Font f = Strip(font);
            return (static_cast<size_t>(cellHeight) << 16) + pointer_cast<uint16_t*>(&f)[0];
        }

    private:
        
        static std::unordered_map<size_t, T *> Fonts_;

		static std::unordered_map<size_t, std::vector<T*>> FallbackFonts_;

    }; 

    template<typename T>
    std::unordered_map<size_t, T *> Font<T>::Fonts_;

	template<typename T>
	std::unordered_map<size_t, std::vector<T*>> Font<T>::FallbackFonts_;

} // namespace tpp


namespace tpp {

    template<typename T>
    class Font {
	public:
		/** Creates a font to fit the given cell dimensions. 
		 */
	    static T * GetOrCreate(ui::Font font, unsigned cellWidth, unsigned cellHeight) {
			unsigned id = FontHash(font, cellHeight);
			auto i = Fonts_.find(id);
			if (i == Fonts_.end()) {
			    i = Fonts_.insert(std::make_pair(id, new T(font, cellWidth, cellHeight))).first;
				// add the font to its fallback fonts as well
				ASSERT(FallbackFonts_.find(id) == FallbackFonts_.end()); // if the font does not exist, its cache should neither
				FallbackFonts_[id].push_back(i->second);
			}
			return i->second;
		}

		/** Note that font destructor does not delete the fallback cache, this should be done by the font manager itself if deleting a font will ever be implemented (otherwise there may be issues with fallback fonts wanting to delete their fallback cache, which they do not have, etc.)
		 */
		virtual ~Font() {
		}

        virtual bool supportsCodepoint(char32_t codepoint) = 0;

		/** Returns the fallback font that can be used for the given UTF codepoint. 
		 */
		T * fallbackFor(unsigned cellWidth, unsigned cellHeight, char32_t codepoint) {
			std::vector<T*> & fallbackCache = FallbackFonts_[FontHash(font_, cellHeight)];
            for (auto i : fallbackCache) {
				if (i != this && i->supportsCodepoint(codepoint))
                    return i;
			}
			// if the character we search the fallback for is double width increase the cell width now
			//cellWidth *= helpers::Char::ColumnWidth(codepoint);
            T * f = new T(*dynamic_cast<T const *>(this), cellWidth, cellHeight, codepoint);
            fallbackCache.push_back(f);
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

		static std::unordered_map<unsigned, std::vector<T*>> FallbackFonts_;

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

		/** Joins the font and the cell height together to form a unique hash for the font & size under which the font and its fallback cache is stored. 
  		 */
		static unsigned FontHash(ui::Font font, unsigned cellHeight) {
			return (cellHeight << 8) + pointer_cast<uint8_t*>(&font)[0];
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

	template<typename T>
	std::unordered_map<unsigned, std::vector<T*>> Font<T>::FallbackFonts_;


} // namespace tpp