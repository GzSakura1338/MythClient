#pragma once

#include "contracts/UiCatalog.h"

namespace nova::menu {

// Install once before constructing a GUI. Reject replacement while rows borrow
// descriptor/property storage, preserving both Classic and Panel references.
bool InstallFeatureCatalog(ui::FeatureCatalog catalog);
const ui::FeatureCatalog& Features();
const ui::Feature* FeatureForToggle(const Properties::AbstractProperty* property);
Properties::PropDesc FeatureToggleRow(const ui::Feature& feature);

}
