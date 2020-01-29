#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)
#include "directwrite_font.h"

namespace tpp {

    HRESULT STDMETHODCALLTYPE DirectWriteFont::TextAnalysis::GetTextAtPosition(UINT32 textPosition, WCHAR const ** textString, UINT32* textLength) {
        if (textPosition == 0) {
            *textString = c_;
            *textLength = 1; // TODO true for surrogate pairs UTF16?
        } else {
            *textString = nullptr;
            *textLength = 0;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFont::TextAnalysis::GetTextBeforePosition(UINT32 textPosition, WCHAR const** textString, _Out_ UINT32* textLength) {
        ASSERT(textPosition == 0);
        *textString = nullptr;  
        *textLength = 0;
        return S_OK;
    }

    DWRITE_READING_DIRECTION STDMETHODCALLTYPE DirectWriteFont::TextAnalysis::GetParagraphReadingDirection() {
        // For now only left to right is supported, should the terminal really support RTL?
        return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFont::TextAnalysis::GetLocaleName(UINT32 textPosition, UINT32* textLength, WCHAR const** localeName) {
        ASSERT(textPosition == 0);
        *localeName = DirectWriteApplication::Instance()->localeName();
        *textLength = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFont::TextAnalysis::GetNumberSubstitution(UINT32 textPosition, UINT32* textLength, IDWriteNumberSubstitution ** numberSubstitution) {
        ASSERT(textPosition == 0);
        *numberSubstitution = nullptr;
        *textLength = 0;
        return S_OK;
    }

}
#endif