#pragma once

#include "helpers/object.h"
#include "helpers/shapes.h"

namespace ui {


	/** Describes a coordinate in the UI framework.
	 */
	typedef unsigned Coord;

	/** Point in the UI framework.
	 */
	typedef helpers::Point<Coord> Point;

	/** Rectangle in the ui framework.
	 */
	typedef helpers::Rect<Coord> Rect;


	/** Describes the canvas of the UI controls.

		The canvas acts as an interface to the underlying window.
	 */
	class Canvas {
	public:
		Coord width() const {
			return width_;
		}

		Coord height() const {
			return height_;
		}


	private:
		Coord width_;
		Coord height_;



	}; // ui::Canvas



	/** Base class for all UI controls. 

	 */
	class Control : public helpers::Object {
	public:
		Control* parent() const {
			return parent_;
		}


	private:
		Control * parent_;

	};
} // namespace ui