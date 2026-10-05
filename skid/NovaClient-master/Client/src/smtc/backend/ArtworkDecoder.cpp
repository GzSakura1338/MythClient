#include "smtc/backend/ArtworkDecoder.h"
#include <shcore.h>
#include <wincodec.h>

namespace nova::smtc::backend {
std::shared_ptr<Artwork> DecodeArtwork(ABI::Windows::Storage::Streams::IRandomAccessStreamReference* reference,
    const std::shared_ptr<Signal>& signal) {
    if (!reference || signal->stop.load()) return {};
    ComPtr<__FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStreamWithContentType> operation;
    Require(reference->OpenReadAsync(&operation));
    if (!Await(operation.Get(), signal)) return {};
    ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStreamWithContentType> content;
    Require(operation->GetResults(&content));
    ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStream> random;
    Require(content.As(&random));
    UINT64 bytes{}; Require(random->get_Size(&bytes));
    if (!bytes || bytes > 8 * 1024 * 1024) return {};
    ComPtr<IStream> stream;
    Require(CreateStreamOverRandomAccessStream(random.Get(), IID_PPV_ARGS(&stream)));
    ComPtr<IWICImagingFactory> factory;
    Require(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
    ComPtr<IWICBitmapDecoder> decoder;
    Require(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder));
    ComPtr<IWICBitmapFrameDecode> frame; Require(decoder->GetFrame(0, &frame));
    UINT width{}, height{}; Require(frame->GetSize(&width, &height));
    if (!width || !height || width > 8192 || height > 8192 || UINT64(width) * height > 32 * 1024 * 1024) return {};
    const double scale = std::min(1.0, 256.0 / std::max(width, height));
    auto art = std::make_shared<Artwork>();
    art->width = std::max(1, static_cast<int>(width * scale)); art->height = std::max(1, static_cast<int>(height * scale));
    ComPtr<IWICBitmapScaler> scaler; Require(factory->CreateBitmapScaler(&scaler));
    Require(scaler->Initialize(frame.Get(), art->width, art->height, WICBitmapInterpolationModeFant));
    ComPtr<IWICFormatConverter> converter; Require(factory->CreateFormatConverter(&converter));
    Require(converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
        nullptr, 0, WICBitmapPaletteTypeCustom));
    art->rgba.resize(static_cast<size_t>(art->width) * art->height);
    Require(converter->CopyPixels(nullptr, art->width * 4, static_cast<UINT>(art->rgba.size() * 4),
        reinterpret_cast<BYTE*>(art->rgba.data())));
    return signal->stop.load() ? nullptr : art;
}
}
