#pragma once
#include "config/settings/Properties.h"

namespace nova::ui::music {
// Keep stable source IDs in configs; both GUI styles render a live source picker.
class SourceProperty final : public Properties::StringProperty {
public:
    SourceProperty() : StringProperty("Auto") {}
};
}
