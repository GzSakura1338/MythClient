#pragma once
#include "smtc/backend/Async.h"
#include "smtc/MediaSnapshot.h"
#include <windows.storage.streams.h>
namespace nova::smtc::backend {
std::shared_ptr<Artwork> DecodeArtwork(ABI::Windows::Storage::Streams::IRandomAccessStreamReference* reference,
    const std::shared_ptr<Signal>& signal);
}
