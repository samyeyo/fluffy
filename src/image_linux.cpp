// src/image_linux.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Image — Linux platform decoder using GdkPixbuf.

#ifdef __linux__

#include <fluffy/image.hpp>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cstdlib>
#include <cstring>

namespace fluffy {
namespace detail {

// ––––––––[ platformLoadImage ] decodes an image file to RGBA pixels via GdkPixbuf.
bool platformLoadImage(const std::string& path,
                       unsigned char*& rgba_out,
                       int& w_out, int& h_out) {
    GError* error = nullptr;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
    if (!pixbuf || error) {
        if (error) g_error_free(error);
        if (pixbuf) g_object_unref(pixbuf);
        return false;
    }

    w_out = gdk_pixbuf_get_width(pixbuf);
    h_out = gdk_pixbuf_get_height(pixbuf);
    if (w_out <= 0 || h_out <= 0) {
        g_object_unref(pixbuf);
        return false;
    }

    const int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    const int rowbytes = w_out * 4;
    const size_t total = static_cast<size_t>(rowbytes) * static_cast<size_t>(h_out);
    rgba_out = static_cast<unsigned char*>(std::malloc(total));
    if (!rgba_out) {
        g_object_unref(pixbuf);
        return false;
    }

    // GdkPixbuf is typically RGB or RGBA. Always write RGBA.
    const guchar* src = gdk_pixbuf_get_pixels(pixbuf);
    const int src_rowstride = gdk_pixbuf_get_rowstride(pixbuf);

    for (int y = 0; y < h_out; ++y) {
        const guchar* row = src + y * src_rowstride;
        unsigned char* dst = rgba_out + y * rowbytes;
        for (int x = 0; x < w_out; ++x) {
            const guchar* px = row + x * n_channels;
            dst[x * 4 + 0] = px[0];  // R
            dst[x * 4 + 1] = px[1];  // G
            dst[x * 4 + 2] = px[2];  // B
            dst[x * 4 + 3] = (n_channels >= 4) ? px[3] : 255;  // A
        }
    }

    g_object_unref(pixbuf);
    return true;
}

}  // namespace detail
}  // namespace fluffy

#endif  // __linux__
