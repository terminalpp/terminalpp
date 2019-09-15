#pragma once

#include "widget.h"
#include "layout.h"


namespace ui {

    using helpers::CreateHandler;

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

	#define PROPERTY_BUILDER(NAME, TYPE, PROPERTY_NAME, BASE_WIDGET) class NAME { \
	    public: \
		    explicit NAME(TYPE value): \
			    value_(value) { \
			} \
		private: \
		    TYPE value_; \
			template<typename WIDGET> \
			friend typename std::enable_if<std::is_base_of<BASE_WIDGET,WIDGET>::value, ui::Builder<WIDGET>>::type \
			operator << (ui::Builder<WIDGET> widget, NAME const & property) { \
			    widget->PROPERTY_NAME(property.value_); \
				return widget; \
			} \
	}

	PROPERTY_BUILDER(Visibility, bool, setVisible, Widget);
	PROPERTY_BUILDER(Focus, bool, setFocus, Widget);
	PROPERTY_BUILDER(FocusStop, bool, setFocusStop, Widget);
	PROPERTY_BUILDER(FocusIndex, size_t, setFocusIndex, Widget);
	PROPERTY_BUILDER(WidthHint, SizeHint const &, setWidthHint, Widget);
	PROPERTY_BUILDER(HeightHint, SizeHint const &, setHeightHint, Widget);
	



	/** Wrapper classes for values so that the << operator can be properly overriden. 
	
	    I.e. we know that a boolean is for visibility and not for enabled, etc. In cases where the type itself is enough of a distincition, no wrapper classes are necessary. 
	 */

    struct GeometrySize {
        int const width;
        int const height;
    };

    struct GeometryFull : public GeometrySize {
        int const x;
        int const y;
		
		GeometryFull(int x, int y, int width, int height):
		    GeometrySize{width,height},
			x(x),
			y(y) {
		}
    };

    inline GeometrySize Geometry(int width, int height) {
        return GeometrySize{width, height};
    }

    inline GeometryFull Geometry(int x, int y, int width, int height) {
        return GeometryFull{width, height, x, y};
    }

	class BackgroundBrushHolder {
	public:
		Brush const value;
	}; // ui::Background

	inline BackgroundBrushHolder Background(Brush const & brush) {
		return BackgroundBrushHolder{brush};
	}

	/** Operator << overloads for differen properties. 

	    The most common overloads are defined here, however overloads specific to particular widgets only are defined in their respective header files. 
	 */

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, GeometrySize g) {
        widget->resize(g.width, g.height);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, GeometryFull g) {
        widget->setPosition(g.x, g.y);
        widget->resize(g.width, g.height);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Rect const& x) {
		widget->resize(x.width(), x.height());
		widget->move(x.left(), x.top());
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, Point const& x) {
		widget->move(x.x, x.y);
		return widget;
	}

	template<typename WIDGET>
	inline Builder<WIDGET> operator << (Builder<WIDGET> widget, BackgroundBrushHolder const& b) {
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
		widget->attachChild(child);
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


	// events

#define EVENT_BUILDER(NAME, PAYLOAD, EVENT_NAME, BASE_WIDGET) class NAME { \
    public: \
	    explicit NAME(helpers::EventHandler<PAYLOAD> const & handler): \
		    handler_(handler) { \
		} \
	private: \
	    helpers::EventHandler<PAYLOAD> handler_; \
		template<typename WIDGET> \
		friend typename std::enable_if<std::is_base_of<BASE_WIDGET, WIDGET>::value, ui::Builder<WIDGET>>::type \
		operator << (ui::Builder<WIDGET> widget, NAME const & e) { \
		    widget->EVENT_NAME += e.handler_; \
			return widget; \
		} \
}
    
	EVENT_BUILDER(OnMouseClick, MouseButtonEvent, onMouseClick, Widget);

}  // namespace ui