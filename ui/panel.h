#pragma once

#include "container.h"

namespace ui {

	/** A simple container with custom background. 

	    TODO in the future, add header, borders around, etc. 
	 */
	class Panel : public Container {
	public:

		Brush const& backrgound() const {
			return background_;
		}

		void setBackground(Brush const& value) {
			if (background_ != value) {
				background_ = value;
    			setForceOverlay(! background_.opaque());
				repaint();
			}
		}

	private:

		Brush background_;
 
	}; // ui::Panel


} // namespace ui