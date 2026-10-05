#include "minecraft/utils/inventory/ChestTitle.h"
#include <cstdio>

int main() {
    int failures = 0;
    auto check = [&](bool condition, const char* name) {
        if (!condition) {
            std::printf("FAIL: %s\n", name);
            ++failures;
        }
    };
    auto matches = [](std::string_view title, bool ender = false) {
        return nova::inventory::MatchesChestTitle(title, "Chest", "Large Chest", "Ender Chest", ender);
    };
    check(matches(""), "unnamed chest");
    check(matches(" \t\r\n"), "blank chest");
    check(matches("\xC2\xA7" "r \xC2\xA7" "8"), "format-only chest");
    check(matches("\xC2\xA7" "aChest\xC2\xA7" "r"), "colored chest");
    check(matches("Chest"), "normal chest");
    check(matches(" Large Chest "), "large chest");
    check(!matches("Ender Chest"), "ender disabled");
    check(matches("Ender Chest", true), "ender enabled");
    check(!matches("Shop"), "named menu rejected");
    check(!matches("Chest Shop"), "substring rejected");
    check(nova::inventory::MatchesChestTitle("\xE7\xAE\xB1\xE5\xAD\x90",
        "\xE7\xAE\xB1\xE5\xAD\x90", "Large Chest", "Ender Chest", false), "translated chest");
    check(!nova::inventory::MatchesChestTitle("Shop", "", "", "", false), "failed translation");
    std::printf("chest title failures=%d\n", failures);
    return failures ? 1 : 0;
}
