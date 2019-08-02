#pragma once

#include "widget.h"

namespace ui {
	/** The UI builder. 

	    The builder just encapsulates an arbitrary widget, retaining its type in a smart pointer like structure. For more details on how it is used see the documentation to the Create functions below. 
	 */
	template<typename WIDGET>
	class Builder {
	public:

		/** Builder must be initialized with a valid widget. 
		 */
		Builder(WIDGET* widget) :
			ptr_(widget) {
		}

		/** Builder acts as a smart pointer. 
		 */
		WIDGET & operator * () {
			return *ptr_;
		}

		WIDGET * operator -> () {
			return ptr_;
		}

		/** Builder implicitly converts to the underlying widget. 
		 */
		operator WIDGET* () {
			return ptr_;
		}

	private:
		WIDGET* ptr_;

	}; // ui::Builder

	/** Creates a builder for given widget. 
	 
	    The Create function should be used to either wrap existing widget in a builder, or to create a builder for new widget of given type, depending on whether the widget is provided or not. This allows creation of both temporary and named widgets. 

		Once a widget builder is created, the << operator can be used to update its various properties. 
	 */
	template<typename WIDGET>
	inline Builder<WIDGET> Create(WIDGET* w) {
		return Builder<WIDGET>(w);
	}

	template<typename WIDGET>
	inline Builder<WIDGET> Create() {
		return Builder<WIDGET>(new WIDGET());
	}

	/** Wrapper classes for values so that the << operator can be properly overriden. 
	
	    I.e. we know that a boolean is for visibility and not for enabled, etc. In cases where the type itself is enough of a distincition, no wrapper classes are necessary. 
	 */

	class Visibility {
	public:
		explicit Visibility(bool value) :
			value(value) {
		}

		bool const value;
	}; // ui::Visibility

	class WidthHint {
	public:
		explicit WidthHint(SizeHint const& value) :
			value(value) {
		}

		SizeHint const value;
	}; // ui::WidthHint

	class HeightHint {
	public:
		explicit HeightHint(SizeHint const& value) :
			value(value) {
		}

		SizeHint const value;
	}; // ui::HeightHint

	class Background {
	public:
		explicit Background(Brush const& brush) :
			value(brush) {
		}

		explicit Background(Color color) :
			value(Brush(color)) {
		}

		Brush const value;
	}; // ui::Background


	/** Operator << overloads for differen properties. 

	    The most common overloads are defined here, however overloads specific to particular widgets only are defined in their respective header files. 
	 */

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Visibility v) {
		widget->setVisible(v.value);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Rect const& x) {
		widget->resize(x.width(), x.height());
		widget->move(x.left, x.top);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Point const& x) {
		widget->move(x.col, x.row);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, WidthHint const& wh) {
		widget->setWidthHint(wh.value);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, HeightHint const& wh) {
		widget->setHeightHint(wh.value);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Background const& b) {
		widget->setBackground(b.value);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Layout* layout) {
		widget->setLayout(layout);
		return widget;
	}

	template<typename WIDGET, typename CHILD>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Builder<CHILD> child) {
		widget->addChild(child);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Font const & font) {
		widget->setFont(font);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, std::string const& str) {
		widget->setCaption(str);
		return widget;
	}


}  // namespace ui