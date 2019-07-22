#pragma once

#include "widget.h"

namespace ui {

	class Label : public Widget {
	public:

		Label(int left = 0, int top = 0, unsigned width = 10, unsigned height = 1) :
			Widget(left, top, width, height),
			text_("Label"),
			textColor_(Color::White()),
			font_(Font()),
			background_(Color::Blue()) {
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

		Color textColor() const {
			return textColor_;
		}

		void setTextColor(Color value) {
			if (textColor_ != value) {
				textColor_ = value;
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

		Color background() const {
			return background_;
		}

		void setBackground(Color value) {
			if (background_ != value) {
				background_ = value;
				repaint();
			}
		}

	protected:

		void paint(Canvas& canvas) {
			canvas.fill(Rect(width(), height()), background_, textColor_, ' ', Font());
			canvas.textOut(Point(0, 0), text_);
		}

	private:

		std::string text_;
		Color textColor_;
		Font font_;
		Color background_;
	};

} // namespace ui