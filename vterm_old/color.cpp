#include <algorithm>

#include "color.h"

namespace vterm {

	Palette Palette::Colors16() {
		return Palette{
			Color::Black(), // 0
			Color::DarkRed(), // 1
			Color::DarkGreen(), // 2
			Color::DarkYellow(), // 3
			Color::DarkBlue(), // 4
			Color::DarkMagenta(), // 5
			Color::DarkCyan(), // 6
			Color::Gray(), // 7
			Color::DarkGray(), // 8
			Color::Red(), // 9
			Color::Green(), // 10
			Color::Yellow(), // 11
			Color::Blue(), // 12
			Color::Magenta(), // 13
			Color::Cyan(), // 14
			Color::White() // 15
		};
	}

	Palette::Palette(std::initializer_list<Color> colors) :
		size_(colors.size()),
		colors_(new Color[colors.size()]) {
		unsigned i = 0;
		for (Color c : colors)
			colors_[i++] = c;
	}

	Palette::Palette(Palette const& from) :
		size_(from.size_),
		colors_(new Color[from.size_]) {
		memcpy(colors_, from.colors_, sizeof(Color) * size_);
	}

	Palette::Palette(Palette&& from) :
		size_(from.size_),
		colors_(from.colors_) {
		from.size_ = 0;
		from.colors_ = nullptr;
	}

	void Palette::fillFrom(Palette const& from) {
		size_t s = std::min(size_, from.size_);
		std::memcpy(colors_, from.colors_, sizeof(Color) * s);
	}

} // namespace vterm