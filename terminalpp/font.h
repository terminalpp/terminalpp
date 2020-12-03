#pragma once

#include <unordered_map>

#include "helpers/helpers.h"
#include "helpers/char.h"

#include "ui/font.h"
#include "ui/geometry.h"

#include "config.h"

namespace tpp {

    /** Rendering font information. 
     */
    class FontMetrics {
    public:

        /** Returns the font associated with the font specification. 
         */
        ui::Font font() const {
            return font_;
        }

        /** The size of single character cell in pixels. 
         */
        ui::Size cellSize() const {
            return cellSize_;
        }

        /** Offset of the font drawing inside the cell. 
         
            This is usually 0 unless special font glyphs are used, or cell size is different from font size. 
         */
        ui::Point offset() const {
            return offset_;
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

        // subclasses should free their native resources here
        virtual ~FontMetrics()  = default;

        static ui::Font Strip(ui::Font f) {
            return f.setUnderline(false).setStrikethrough(false).setBlink(false);
        }

    protected:

        FontMetrics(ui::Font const & font, ui::Size fontSize):
            font_{Strip(font)},
            fontSize_{fontSize.width() * font_.width(), fontSize.height() * font_.height()} {
        }

        ui::Font font_;

        ui::Size fontSize_;

        ui::Size cellSize_;

        // offset of the character in the cell if the characters need to be centered (for fallback fonts)
        ui::Point offset_;

        float ascent_;
        float underlineOffset_;
        float underlineThickness_;
        float strikethroughOffset_;
        float strikethroughThickness_;

    }; // tpp::FontMetrics

    /** Base template for fonts used in the terminal window renderers. 
     
        Handles the mechanism for caching the already configured fonts in different sizes and their fallback alternatives for various characters

        Implementations should only pay attention (and configure if required) the fontSize_ internal member, which represents the size of the cell as required by the selected font. The font class template then handles the update of the actual cell size as per the char and line spacings.  
     */
    template<typename T>
    class Font : public FontMetrics {
    public:

        /** Returns a font of given font height and calculates the cell size according to the settings. 
         
            This is to be used to determine the base cell width and height. 
         */
        static T * Get(ui::Font font, int fontHeight) {
            size_t id = CreateIdFrom(font, fontHeight);
            auto i = Fonts_.find(id);
            if (i == Fonts_.end()) {
                T * f = new T(font, fontHeight, 0);
                f->adjustCellSize();
                i = Fonts_.insert(std::make_pair(id, f)).first;
            }
            return i->second;
        }

        /** Returns a font that fits given cell size. 
         
            This function is to be used when the cell size of the font should not change, but new font must be selected. 
         */
        static T * Get(ui::Font font, ui::Size cellSize) {
            ui::Size fontSize = GetFontSize(cellSize);
            size_t id = CreateIdFrom(font, fontSize.height());
            auto i = Fonts_.find(id);
            if (i == Fonts_.end()) {
                T * f = new T(font, fontSize.height(), fontSize.width());
                f->adjustCellSize();
                i = Fonts_.insert(std::make_pair(id, f)).first;
            }
            return i->second;
        }

        /** Returns a font that provides fallback for given character codepoint.

            Always returns a font, but if a suitable callback cannot be found, the returned font will not render the character properly.  
         */
        T * fallbackFor(char32_t codepoint) {
			std::vector<T*> & fallbackCache = FallbackFonts_[CreateIdFrom(font_, fontSize_.height())];
            for (auto i : fallbackCache) {
				if (i != this && i->supportsCodepoint(codepoint))
                    return i;
			}
			// if the character we search the fallback for is double width increase the cell width now
			//cellWidth *= Char::ColumnWidth(codepoint);
            T * f = new T(*static_cast<T const *>(this), codepoint);
            f->adjustCellSize();
            fallbackCache.push_back(f);
            return f;
        }

    protected:

        Font(ui::Font font, ui::Size cellSize):
            FontMetrics(font, cellSize) {
        }

        static size_t CreateIdFrom(ui::Font font, int cellHeight) {
            ui::Font f = Strip(font);
            return (static_cast<size_t>(cellHeight) << 16) + pointer_cast<uint16_t*>(&f)[0];
        }

        /** Adjusts the cell size of the font. 
         
            Adjusts the cell size and offset according to the character and line spacing in the configuration. 
         */
        void adjustCellSize() {
            Config & config = Config::Instance();
            double wm = config.renderer.font.charSpacing();
            double hm = config.renderer.font.lineSpacing();
            cellSize_ = ui::Size{
                static_cast<int>(fontSize_.width() * wm),
                static_cast<int>(fontSize_.height() * hm),
            };
            offset_ += ui::Point{
                (cellSize_.width() - fontSize_.width()) / 2, 
                (cellSize_.height() - fontSize_.height()) / 2
            };
        }

        /** Calculates the font size from cell size based on the character and line spacing settings. 
         */
        static ui::Size GetFontSize(ui::Size cellSize) {
            Config & config = Config::Instance();
            double wm = config.renderer.font.charSpacing();
            double hm = config.renderer.font.lineSpacing();
            int w = static_cast<int>(cellSize.width() / wm);
            int h = static_cast<int>(cellSize.height() / hm);
            if (w * wm < cellSize.width())
                ++w;
            if (h * hm < cellSize.height())
                ++h;
            return ui::Size{w, h};
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
