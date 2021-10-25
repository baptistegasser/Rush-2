#pragma once

#include <stdint.h>
#include <string>

namespace PM3D {

	class Image {
	public:
		using value_type = uint8_t;
		using size_type = unsigned int;

	private:
		size_type m_width, m_height;
		value_type* m_data;

	public:
		Image(const std::string& path);
		~Image();

		size_type width() const noexcept;
		size_type height() const noexcept;
		size_type size() const noexcept;

		const value_type* data() const noexcept;
		value_type greyScaleAt(const int& x, const int& y) const noexcept;
	};

} // namespace PM3D
