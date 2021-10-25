#include "stdafx.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "util/stb_image.h"

#include <stdexcept>

namespace PM3D {

    Image::Image(const std::string& path)
    {
        stbi_set_flip_vertically_on_load(true);

        int w = 0, h = 0;
        m_data = stbi_load(path.c_str(), &w, &h, nullptr, 1);
        m_width = static_cast<size_type>(w);
        m_height = static_cast<size_type>(h);

        if (m_data == nullptr) {
            throw std::runtime_error(stbi_failure_reason());
        }
    }

    Image::~Image()
    {
        stbi_image_free(m_data);
    }

    Image::size_type Image::width() const noexcept
    {
        return m_width;
    }

    Image::size_type Image::height() const noexcept
    {
        return m_height;
    }

    // Return the number of pixels
    Image::size_type Image::size() const noexcept
    {
        return m_width * m_height;
    }

    const Image::value_type* Image::data() const noexcept
    {
        return m_data;
    }

    Image::value_type Image::greyScaleAt(const int& x, const int& y) const noexcept
    {
        const int pos = (x * m_width + y);
        return m_data[pos];
    }

} // namespace PM3D
