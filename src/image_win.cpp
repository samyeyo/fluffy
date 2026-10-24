// src/image_win.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Image — Windows platform decoder using WIC (Windows Imaging Component).

#ifdef _WIN32

#include <fluffy/image.hpp>

#include <wincodec.h>
#include <wrl/client.h>
#include <cstdlib>
#include <new>

using Microsoft::WRL::ComPtr;

namespace fluffy {
namespace detail {

// ––––––––[ platformLoadImage ] decodes an image file to RGBA pixels via WIC.
bool platformLoadImage(const std::string& path,
                       unsigned char*& rgba_out,
                       int& w_out, int& h_out) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool com_owned = SUCCEEDED(hr);

    ComPtr<IWICImagingFactory> factory;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        if (com_owned) CoUninitialize();
        return false;
    }

    // Convert path to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
        if (com_owned) CoUninitialize();
        return false;
    }
    auto wpath = std::make_unique<wchar_t[]>(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.get(), wlen);

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(
        wpath.get(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) {
        if (com_owned) CoUninitialize();
        return false;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        if (com_owned) CoUninitialize();
        return false;
    }

    frame->GetSize(
        reinterpret_cast<UINT*>(&w_out),
        reinterpret_cast<UINT*>(&h_out));
    if (w_out <= 0 || h_out <= 0) {
        if (com_owned) CoUninitialize();
        return false;
    }

    // Convert to 32bpp RGBA
    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        if (com_owned) CoUninitialize();
        return false;
    }

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr, 0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        if (com_owned) CoUninitialize();
        return false;
    }

    const UINT row_bytes = static_cast<UINT>(w_out) * 4;
    const UINT total = row_bytes * static_cast<UINT>(h_out);
    rgba_out = static_cast<unsigned char*>(std::malloc(total));
    if (!rgba_out) {
        if (com_owned) CoUninitialize();
        return false;
    }

    hr = converter->CopyPixels(nullptr, row_bytes, total, rgba_out);
    if (FAILED(hr)) {
        std::free(rgba_out);
        rgba_out = nullptr;
        if (com_owned) CoUninitialize();
        return false;
    }

    if (com_owned) CoUninitialize();
    return true;
}

}  // namespace detail
}  // namespace fluffy

#endif  // _WIN32
