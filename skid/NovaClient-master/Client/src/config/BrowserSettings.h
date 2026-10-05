#pragma once
#include <string>
namespace nova::config {
struct BrowserSettings {std::string home="about:blank";int fps=30,zoom=100;bool audio=false;};
BrowserSettings LoadBrowserSettings();
void SaveBrowserSettings(const BrowserSettings& settings);
struct BrowserPlacement { int x=1800,y=1200,width=7500,height=7500; }; // 1/10000 viewport units
BrowserPlacement LoadBrowserPlacement();
void SaveBrowserPlacement(const BrowserPlacement&);
}
