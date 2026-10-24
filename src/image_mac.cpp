// src/image_mac.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Image — macOS platform decoder using CGImage via ObjC runtime.
// We forward-declare the minimal CoreGraphics/ImageIO types we need to
// avoid including Apple framework headers that use @import (incompatible
// with C++ compilation).  CoreGraphics.framework is already linked
// transitively through GLFW.

#ifdef __APPLE__

#include <fluffy/image.hpp>

#include <objc/runtime.h>
#include <objc/message.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

// ––––––––[ ObjC runtime forward declarations ] ––––––––
extern "C" {
id objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void* pool);
}

// ––––––––[ CoreGraphics / ImageIO forward declarations ] ––––––––
typedef struct CGImage*        CGImageRef;
typedef struct CGColorSpace*   CGColorSpaceRef;
typedef struct CGContext*      CGContextRef;
typedef struct CGImageSource*  CGImageSourceRef;
typedef unsigned long          CGBitmapInfo;

enum {
    kCGImageAlphaPremultipliedLast = 1,
};

#if __LP64__
typedef double CGFloat;
#else
typedef float CGFloat;
#endif

struct CGRect { CGFloat x, y, w, h; };
static inline CGRect CGRectMake(CGFloat x, CGFloat y, CGFloat w, CGFloat h) {
    return {x, y, w, h};
}

extern "C" {
CGImageRef       CGImageSourceCreateImageAtIndex(CGImageSourceRef src, size_t i, void* opts);
CGImageSourceRef CGImageSourceCreateWithURL(void* url, void* opts);
size_t           CGImageGetWidth(CGImageRef img);
size_t           CGImageGetHeight(CGImageRef img);
void             CFRelease(void* cf);
CGColorSpaceRef  CGColorSpaceCreateDeviceRGB(void);
CGContextRef     CGBitmapContextCreate(void* data, size_t w, size_t h,
                         size_t bpc, size_t bpr, CGColorSpaceRef cs,
                         CGBitmapInfo info);
void CGContextDrawImage(CGContextRef ctx, CGRect rect, CGImageRef img);
void CGContextRelease(CGContextRef ctx);
void CGColorSpaceRelease(CGColorSpaceRef cs);
}

namespace fluffy {
namespace detail {

// ––––––––[ autoreleasepool ] RAII guard.
struct AutoreleasePool {
    id pool;
    AutoreleasePool() : pool(objc_autoreleasePoolPush()) {}
    ~AutoreleasePool() { objc_autoreleasePoolPop(pool); }
};

// ––––––––[ platformLoadImage ] decodes an image file to RGBA pixels via CGImage.
bool platformLoadImage(const std::string& path,
                       unsigned char*& rgba_out,
                       int& w_out, int& h_out) {
    AutoreleasePool pool;

    Class NSString = objc_getClass("NSString");
    SEL allocSel  = sel_registerName("alloc");
    SEL initUTF8  = sel_registerName("initWithUTF8String:");

    // NSString* nsPath = [[NSString alloc] initWithUTF8String:path.c_str()]
    id nsStr = reinterpret_cast<id(*)(id, SEL)>(objc_msgSend)(
        reinterpret_cast<id>(NSString), allocSel);
    id nsPath = reinterpret_cast<id(*)(id, SEL, const char*)>(objc_msgSend)(
        nsStr, initUTF8, path.c_str());

    Class NSURL = objc_getClass("NSURL");
    SEL fileURLWithPath = sel_registerName("fileURLWithPath:");

    // NSURL* url = [NSURL fileURLWithPath:nsPath]  (autoreleased class factory)
    id url = reinterpret_cast<id(*)(id, SEL, id)>(objc_msgSend)(
        reinterpret_cast<id>(NSURL), fileURLWithPath, nsPath);

    // CGImageSourceRef src = CGImageSourceCreateWithURL(...)
    CGImageSourceRef src = CGImageSourceCreateWithURL(
        reinterpret_cast<void*>(url), nullptr);
    if (!src) return false;

    CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, nullptr);
    CFRelease(src);
    if (!img) return false;

    w_out = static_cast<int>(CGImageGetWidth(img));
    h_out = static_cast<int>(CGImageGetHeight(img));
    if (w_out <= 0 || h_out <= 0) {
        CFRelease(img);
        return false;
    }

    const size_t bytes_per_row = static_cast<size_t>(w_out) * 4;
    const size_t total = bytes_per_row * static_cast<size_t>(h_out);
    rgba_out = static_cast<unsigned char*>(std::malloc(total));
    if (!rgba_out) {
        CFRelease(img);
        return false;
    }

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        rgba_out,
        static_cast<size_t>(w_out),
        static_cast<size_t>(h_out),
        8,
        bytes_per_row,
        cs,
        kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(cs);

    if (!ctx) {
        std::free(rgba_out);
        rgba_out = nullptr;
        CFRelease(img);
        return false;
    }

    CGContextDrawImage(ctx, CGRectMake(0, 0,
        static_cast<CGFloat>(w_out),
        static_cast<CGFloat>(h_out)), img);
    CGContextRelease(ctx);
    CFRelease(img);

    // CGBitmapContext has a bottom-left origin, so rows arrive bottom-up.
    // Flip them to the top-down order WIC and GdkPixbuf produce.
    for (int y = 0; y < h_out / 2; ++y) {
        unsigned char* top = rgba_out + static_cast<size_t>(y) * bytes_per_row;
        unsigned char* bottom = rgba_out +
            static_cast<size_t>(h_out - 1 - y) * bytes_per_row;
        std::swap_ranges(top, top + bytes_per_row, bottom);
    }

    return true;
}

}  // namespace detail
}  // namespace fluffy

#endif  // __APPLE__
