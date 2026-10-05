#pragma once
#include "minecraft/java/api/base.h"
#include <string>
namespace nova::java {
enum class AnnouncementSource { Chat, Title, Subtitle, ActionBar };
struct Announcement { AnnouncementSource source=AnnouncementSource::Chat; std::string text; };
// Reads a newly received server announcement, never chat history or outgoing text.
bool ReadAnnouncement(jobject packet,Announcement& out);
}
