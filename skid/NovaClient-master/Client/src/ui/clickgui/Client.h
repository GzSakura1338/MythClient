#pragma once

#include <vector>
#include "imgui.h"
#include "contracts/UiCatalog.h"
#include "property/Properties.h"

namespace Client
{
	struct FeatureGroup
	{
		const char* name;
		float height;
		std::vector<Properties::PropDesc> features;
	};
	struct FeatureSubPage
	{
		const char* name;
		std::vector<FeatureGroup> groups;
	};
	struct FeaturePage
	{
		const char* name;
		std::vector<FeatureSubPage> subpages;
	};
	struct FeatureCategory
	{
		char8_t icon[4];
		nova::ui::IconFace face;
		const char* name;
		std::vector<FeaturePage> pages;
	};

	extern const std::vector<FeatureCategory>& FeatureCategories;
}
