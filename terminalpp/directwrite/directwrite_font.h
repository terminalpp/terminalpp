#pragma once 
#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE) 

#include "helpers/string.h"

#include "../config.h"

#include "../font.h"
#include "directwrite_application.h"

namespace tpp {


    /** Font implementation for DirectWrite rendering.
     
        
     */
    class DirectWriteFont : public Font<DirectWriteFont> {
    public:

	    float sizeEm() const {
			return sizeEm_;
		}

        IDWriteFontFace * fontFace() const {
            return fontFace_.Get();
        }

		bool supportsCodepoint(char32_t codepoint) {
			UINT16 gi;
			UINT32 cp = codepoint;
			fontFace_->GetGlyphIndices(&cp, 1, &gi);
			return (gi != 0);
		}

    private:

        friend class Font<DirectWriteFont>;

        class TextAnalysis;

        /** Creates a font corresponding to the given ui::Font and cell height. 
         
            This is a two-stage process, where first the closest typeface is selected and once known, the font specification is determined from the typeface.
         */
        DirectWriteFont(ui2::Font font, int cellHeight, int cellWidth = 0);

        DirectWriteFont(DirectWriteFont const & base, char32_t codepoint);

        /** Given a typeface, determines the font metrics to fit the specified cell dimensions. 
         
            If cellWidth is 0, then the cell width is determined by the font metrics at given cell height, otherwise the font size is resized and centered to fit the fully specified cell. 
         */
        void initializeFromFontFace();

        /** Size of the font face to be used when rendering. 
         */
        float sizeEm_;

        /** The DirectWrite font face. */
		Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace_;

    }; // DirectWriteFont

} // namespace tpp

#endif