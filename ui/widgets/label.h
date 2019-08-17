#pragma once

#include "../widget.h"
#include "../builders.h"

namespace ui {

	class Label : public PublicWidget {
	public:

		Label() :
			PublicWidget(),
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

		Brush background() const {
			return background_;
		}

		void setBackground(Brush const & value) {
			if (background_ != value) {
				background_ = value;
				setForceOverlay(! background_.color.opaque());
				repaint();
			}
		}

	protected:

		void paint(Canvas& canvas) override {
			canvas.fill(Rect(width(), height()), background_);
			canvas.textOut(Point(0, 0), text_, textColor_, font_);
		}

		void mouseClick(int x, int y, MouseButton button, Key modifiers) override {
			Widget::mouseClick(x, y, button, modifiers);
		}

		void mouseDoubleClick(int x, int y, MouseButton button, Key modifiers) override {
			Widget::mouseClick(x, y, button, modifiers);
		}

		void mouseEnter() override {
			repaint();
			Widget::mouseEnter();
		}

		void mouseLeave() override {
			repaint();
			Widget::mouseLeave();
		}


	private:

		std::string text_;
		Color textColor_;
		Font font_;
		Brush background_;
	};

	/** Builder overload for setting the text of a label. 

	    TODO make this enable for any label children
	 */
	template<>
	Builder<Label> operator << (Builder<Label> widget, std::string const& str) {
		widget->setText(str);
		return widget;
	}

} // namespace ui