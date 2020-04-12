#pragma once

#include "../widget.h"
#include "../builders.h"

#include "../traits/box.h"

namespace ui {

	class Label : public PublicWidget, public ui::Box<Label> {
	public:

		Label() :
			PublicWidget{},
			Box{},
			text_{"Label"},
			color_{Color::White},
			font_{} {
		}

		std::string const& text() const {
			return text_;
		}

		std::string& text() {
			return text_;
		}

		void setText(std::string const& value) {
			if (text_ != value) {
				text_ = value;
				repaint();
			}
		}

		Color color() const {
			return color_;
		}

		void setColor(Color value) {
			if (color_ != value) {
				color_ = value;
				repaint();
			}
		}

		Font font() const {
			return font_;
		}

		void setFont(Font const& font) {
			if (font_ != font) {
				font_ = font;
				repaint();
			}
		}

	protected:

		void paint(Canvas& canvas) override {
			canvas.fill(Rect::FromWH(width(), height()), background_);
			canvas.setFg(color_).setFont(font_);
			canvas.textOut(canvas.rect(), text_, HorizontalAlign::Left);
		}

	private:

		std::string text_;
		Color color_;
		Font font_;
	};

} // namespace ui