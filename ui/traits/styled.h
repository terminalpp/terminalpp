#pragma once

namespace ui {

    enum class SemanticStyle {
        None,
        Primary,
        Secondary,
        Success,
        Danger,
        Warning,
        Info
    };

    template<typename T>
    class Styled : public TraitBase<Styled, T> {
    public:

        SemanticStyle semanticStyle() const {
            return semanticStyle_;
        }

        virtual void setSemanticStyle(SemanticStyle value) {
            if (semanticStyle_ != value) {
                semanticStyle_ = value;
                if (semanticStyle_ != SemanticStyle::None)
                    restyle();
                downcastThis()->repaint();
            }
        }

    protected:
        using TraitBase<Styled, T>::downcastThis;

        Styled():
            semanticStyle_{SemanticStyle::None} {
        }

        virtual void restyle() = 0;


        Color styleBackground() {
            switch (semanticStyle_) {
                case SemanticStyle::Primary:
                    return Color::DarkBlue;
                case SemanticStyle::Secondary:
                    return Color::DarkGray;
                case SemanticStyle::Success:
                    return Color::DarkGreen;
                case SemanticStyle::Danger:
                    return Color::DarkRed;
                case SemanticStyle::Warning:
                    return Color::DarkYellow;
                case SemanticStyle::Info:
                    return Color::DarkCyan;
                default:
                    return Color::DarkGray;
            }
        }

        Color styleHighlightBackground() {
            switch (semanticStyle_) {
                case SemanticStyle::Primary:
                    return Color::Blue;
                case SemanticStyle::Secondary:
                    return Color::Gray;
                case SemanticStyle::Success:
                    return Color::Green;
                case SemanticStyle::Danger:
                    return Color::Red;
                case SemanticStyle::Warning:
                    return Color::Yellow;
                case SemanticStyle::Info:
                    return Color::Cyan;
                default:
                    return Color::Gray;
            }
        }

    private:
        SemanticStyle semanticStyle_;

    }; 



} // namespace ui