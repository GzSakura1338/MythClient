#include "ui/clickgui/FeatureTree.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "core/Instance.h"
#include "ui/clickgui/Client.h"

namespace nova::menu {
namespace {

struct CatalogState {
    ui::FeatureCatalog catalog;
    bool installed = false;
};

CatalogState& State() { return Instance<CatalogState>(); }

std::deque<std::string>& LabelArena() {
    return Instance<std::deque<std::string>>();
}

const char* Intern(std::string text) {
    return LabelArena().emplace_back(std::move(text)).c_str();
}

std::vector<Client::FeatureCategory>& Storage() {
    return Instance<std::vector<Client::FeatureCategory>>();
}

struct Bucket {
    std::string page;
    std::string subpage;
    std::vector<const ui::Feature*> features;
};

std::vector<Client::FeaturePage> BuildPages(const std::vector<const ui::Feature*>& features) {
    std::vector<Bucket> buckets;
    for (const auto* feature : features) {
        const std::string subpage = feature->hud ? "HUD" : feature->subpage;
        auto found = std::ranges::find_if(buckets, [feature, &subpage](const Bucket& bucket) {
            return bucket.page == feature->page && bucket.subpage == subpage;
        });
        if (found == buckets.end())
            buckets.push_back(Bucket{feature->page, subpage, {feature}});
        else
            found->features.push_back(feature);
    }

    std::vector<Client::FeaturePage> pages;
    for (const Bucket& bucket : buckets) {
        auto page = std::ranges::find_if(pages, [&bucket](const Client::FeaturePage& candidate) {
            return bucket.page == candidate.name;
        });
        if (page == pages.end()) {
            pages.push_back(Client::FeaturePage{Intern(bucket.page), {}});
            page = std::prev(pages.end());
        }

        std::vector<Client::FeatureGroup> groups;
        std::vector<Properties::PropDesc> rows;
        rows.reserve(bucket.features.size());
        for (const auto* feature : bucket.features)
            rows.push_back(FeatureToggleRow(*feature));
        groups.push_back(Client::FeatureGroup{Intern(bucket.subpage), -1.0f, std::move(rows)});
        page->subpages.push_back(Client::FeatureSubPage{Intern(bucket.subpage), std::move(groups)});
    }
    return pages;
}

std::vector<Client::FeatureCategory> BuildTree() {
    const auto& catalog = Features();

    std::vector<Client::FeatureCategory> tree;
    tree.reserve(catalog.categories.size());
    for (const auto& category : catalog.categories) {
        std::vector<const ui::Feature*> inCategory;
        for (const auto& feature : catalog.features) {
            if (feature.category == category.id)
                inCategory.push_back(&feature);
        }

        std::vector<Client::FeaturePage> pages = BuildPages(inCategory);
        if (pages.empty()) {
            pages.push_back(Client::FeaturePage{
                Intern("General"),
                {Client::FeatureSubPage{Intern("General"), {}}},
            });
        }

        Client::FeatureCategory entry{};
        std::copy_n(category.icon.c_str(), std::min(category.icon.size(), sizeof(entry.icon) - 1), entry.icon);
        entry.face = category.face;
        entry.name = Intern(category.name);
        entry.pages = std::move(pages);
        tree.push_back(std::move(entry));
    }
    return tree;
}

}

Properties::PropDesc FeatureToggleRow(const ui::Feature& feature) {
    std::vector<Properties::PropDesc> parameters;
    for (const auto& row : feature.settings) {
        if (row.getProp() != feature.enabled) parameters.push_back(row);
    }
    return Properties::PropDesc(feature.name.c_str(), feature.enabled, std::move(parameters));
}

const ui::Feature* FeatureForToggle(const Properties::AbstractProperty* property) {
    if (property == nullptr) return nullptr;
    for (const auto& feature : Features().features)
        if (feature.enabled == property) return &feature;
    return nullptr;
}

const ui::FeatureCatalog& Features() { return State().catalog; }

bool InstallFeatureCatalog(ui::FeatureCatalog catalog) {
    auto& state = State();
    if (state.installed) return false;
    for (size_t i = 0; i < catalog.categories.size(); ++i) {
        const auto& category = catalog.categories[i];
        if (category.id.empty()) return false;
        for (size_t j = 0; j < i; ++j)
            if (catalog.categories[j].id == category.id) return false;
    }
    for (size_t i = 0; i < catalog.features.size(); ++i) {
        const auto& feature = catalog.features[i];
        if (feature.id.empty() || !feature.enabled) return false;
        if (std::ranges::none_of(catalog.categories, [&](const auto& c) { return c.id == feature.category; }))
            return false;
        for (size_t j = 0; j < i; ++j)
            if (catalog.features[j].id == feature.id || catalog.features[j].enabled == feature.enabled) return false;
    }
    state.catalog = std::move(catalog);
    Storage() = BuildTree();
    state.installed = true;
    return true;
}

}

namespace Client {
const std::vector<FeatureCategory>& FeatureCategories = nova::menu::Storage();
}
