#pragma once

#include <unordered_map>

#include "helpers/helpers.h"
#include "helpers/char.h"

#include "ui/geometry.h"

namespace tpp {

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

        Font(ui2::Font font, int cellHeight, int cellWidth):
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
