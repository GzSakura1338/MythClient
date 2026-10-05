#include "ui/backend/blur/Blur.h"
#include "ui/clickgui/Menu.h"

#include "FatalWidgets.h"
#include "ui/clickgui/Client.h"
#include "ui/hud/musichud/SourcePicker.h"
#include "ui/clickgui/FeatureTree.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include <vector>
#include "imgui_internal.h"
#include "ui/overlay/WindowPlacement.h"
#include "bind/BindCapture.h"
#include "bind/BindProperty.h"
#include "bind/KeyName.h"

extern ImFont* Medium12;
extern ImFont* ExtraBold20;
extern ImFont* IconFont;
extern ImFont* IconFont2;

namespace
{
	ImFont* FaceFont(nova::ui::IconFace face)
	{
		return face == nova::ui::IconFace::Frost ? IconFont2 : IconFont;
	}

    void ModuleContextMenu(const Properties::PropDesc& feature) {
        const auto* entry = nova::menu::FeatureForToggle(feature.getProp());
        if (!entry || !entry->hiddenInArrayList) return;
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("ModuleMenu");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xFF1C1C1C);
        ImGui::PushStyleColor(ImGuiCol_Border, 0xFF222222);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFFCCCCCC);
        if (nova::ui::blur::BeginPopup("ModuleMenu")) {
            ImGui::TextUnformatted("Hide in ArrayList");
            ImGui::SameLine(0, 16);
            bool hidden = entry->hiddenInArrayList->get();
            if (FatalWidgets::CheckBox("##hidden", &hidden, ImVec2(14, 14)))
                entry->hiddenInArrayList->set(hidden);
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

	namespace Menu
	{
		bool initialized = false;
		std::vector<Client::FeatureCategory> categories;

		int activeCateIndex = 0;
		int activePageIndex = 0;
		int activeSubPageIndex = 0;

		struct PropConfigurerElement
		{
			virtual ImVec2 getSize() const = 0;
			virtual void render(void* propIn) const = 0;

			virtual ~PropConfigurerElement() = default;
		};

		struct CheckBoxElement : PropConfigurerElement
		{
			ImVec2 getSize() const override
			{
				return ImVec2 { 14, 14 };
			}
			void render(void* propIn) const override
			{
				const auto prop = static_cast<Properties::BoolProperty*>(propIn);
				auto val = prop->get();
				if (FatalWidgets::CheckBox("CheckBox", &val, getSize()))
					prop->set(val);
			}
		};

		struct SliderElement : PropConfigurerElement
		{
			ImVec2 getSize() const override
			{
				return ImVec2{ 80, 16 };
			}
			void render(void* propIn) const override
			{
				const auto prop = static_cast<Properties::NumberProp*>(propIn);
				auto val = prop->get();
				if (FatalWidgets::DoubleSlider("Slider", prop->getMin(), prop->getMax(), &val, getSize(), [prop](double val) { return prop->formatValue(val); }))
					prop->set(Properties::NumberProp::processEx(val, DBL_MIN, DBL_MAX, prop->getStep()));
			}
		};

		struct ComboBoxElement : PropConfigurerElement
		{
			ImVec2 getSize() const override
			{
				return ImVec2{ 80, 22 };
			}
			void render(void* propIn) const override
			{
				const auto prop = static_cast<Properties::ModeProperty*>(propIn);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4));
				ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4);
				ImGui::PushStyleColor(ImGuiCol_Border, 0xFF262626);
				ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xFF1A1A1A);
				ImGui::PushStyleColor(ImGuiCol_Header, 0xFF242424);
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, 0xFF242424);
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, 0xFF262626);
				if (FatalWidgets::ComboBox("ComboBox", "ModesPopup",
					prop->getValueNameCStr(prop->get()), getSize()))
				{
					for (int i = 0; i < prop->getValueCount(); ++i)
					{
						const bool selected = i == prop->get();
						ImGui::PushID(i);
						if (selected) ImGui::PushStyleColor(ImGuiCol_Text, 0xFF6666FF);
						if (ImGui::Selectable(prop->getValueNameCStr(i), selected))
							prop->set(i);
						if (selected) ImGui::PopStyleColor();
						ImGui::PopID();
					}
					ImGui::EndPopup();
				}
				ImGui::PopStyleColor(5);
				ImGui::PopStyleVar(2);
			}
		};

		CheckBoxElement CheckBoxElementImpl = CheckBoxElement{};
		struct TextElement : PropConfigurerElement {
			ImVec2 getSize() const override { return {180,20}; }
			void render(void* raw) const override {
				auto& prop=*static_cast<Properties::StringProperty*>(raw);
				std::array<char,1024> buffer{};const auto text=prop.get();
				std::copy_n(text.data(),std::min(text.size(),buffer.size()-1),buffer.data());
				ImGui::SetNextItemWidth(180);
				if(ImGui::InputText("##text",buffer.data(),buffer.size()))prop.set(buffer.data());
			}
		} TextElementImpl;
        struct MediaSourceElement : PropConfigurerElement {
            ImVec2 getSize() const override { return {180,20}; }
            void render(void* raw) const override {
                nova::ui::music::DrawSourceSelector(*static_cast<nova::ui::music::SourceProperty*>(raw),1.f);
            }
        } MediaSourceElementImpl;
        struct KeyElement : PropConfigurerElement {
            ImVec2 getSize() const override { return {80, 22}; }
            void render(void* raw) const override {
                auto& prop = *static_cast<nova::bind::BindProperty*>(raw);
                auto& capture = nova::bind::BindCapture::Get();
                const bool listening = capture.target() == &prop;
                const auto label = listening ? std::string("LISTENING") : nova::bind::KeyName(prop.keybind());
                ImGui::PushStyleColor(ImGuiCol_Button, 0xFF242424);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF2A2A2A);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF303030);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
                ImGui::Button((label + "###key").c_str(), getSize());
                capture.setPressing(ImGui::IsItemActive());
                if (ImGui::IsItemActivated()) {
                    if (listening) capture.Cancel(); else capture.Begin(&prop);
                }
                ImGui::PopStyleVar(); ImGui::PopStyleColor(3);
            }
        } KeyElementImpl;
		SliderElement   SliderElementImpl   = SliderElement{};
		ComboBoxElement ComboBoxElementImpl = ComboBoxElement{};

		PropConfigurerElement* getPropElement(Properties::AbstractProperty* prop)
		{
            if (auto* bind = dynamic_cast<nova::bind::BindProperty*>(prop); bind && bind->keyOnly()) return &KeyElementImpl;
            if (dynamic_cast<nova::ui::music::SourceProperty*>(prop)) return &MediaSourceElementImpl;
			if (dynamic_cast<Properties::StringProperty*>(prop)) return &TextElementImpl;
			if (dynamic_cast<Properties::BoolProperty*>(prop))
				return &CheckBoxElementImpl;
			if (dynamic_cast<Properties::NumberProp*>(prop))
				return &SliderElementImpl;
			if (dynamic_cast<Properties::ModeProperty*>(prop))
				return &ComboBoxElementImpl;
			return nullptr;
		}

		void initialize()
		{
			categories = Client::FeatureCategories;
			initialized = true;
		}
	}
}

void Fatal::Menu::draw()
{
	if (!::Menu::initialized)
		::Menu::initialize();
	ImGui::SetNextWindowSize(ImVec2(848, 588));
	nova::ui::PlaceWindowCentered("FatalMenu", 848.0f, 588.0f);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

	ImGui::Begin(
		"FatalMenu", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize
	);
	nova::ui::CaptureWindow("FatalMenu");
	const auto draws = ImGui::GetWindowDrawList();
	const auto winRectMin = ImGui::GetWindowPos();
	const auto winRectSize = ImGui::GetWindowSize();
	const auto winRectMax = winRectMin + winRectSize;

	nova::ui::blur::LayerBackground(draws, winRectMin, winRectMax, 0xFF151515, 6);

	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 0));
		ImGui::BeginChild("TitleBar", ImVec2(winRectSize.x, 50), ImGuiChildFlags_AlwaysUseWindowPadding);
		{
			const auto fr = FatalWidgets::Rendering::GetDefaultTextRenderer();
			const auto titleDraws = ImGui::GetWindowDrawList();
			nova::ui::blur::LayerBackground(titleDraws, ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), 0xFF1A1A1A, 6, ImDrawFlags_RoundCornersTop);

			ImGui::PushFont(ExtraBold20);
			const auto clientName = nova::menu::panel::ClientSettings::Get().clientName();
			const auto text = clientName.c_str();
			FatalWidgets::Rendering::DrawFatalTitle(
				text,
				ImGui::GetCursorScreenPos() + ImVec2(0, (ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight()) / 2),
				fmodf(ImGui::GetTime(), 1.0f), 1,
				0xFFFFFFFF, 0xFF0000FF, 0xFFFF0000
			);
			ImGui::SameLine(0, fr->CalcTextSize(text).x + 20);
			ImGui::PopFont();

			for (int i = 0; i < ::Menu::categories.size(); ++i)
			{
				if (i > 0) ImGui::SameLine(0, 2);
				const auto& cate = ::Menu::categories.at(i);
				ImGui::SetCursorPosY(11);
				if (FatalWidgets::IconButton(cate.name, reinterpret_cast<const char*>(cate.icon), cate.name, ::Menu::activeCateIndex == i, FaceFont(cate.face), ImVec2(0, 28)))
				{
					::Menu::activeCateIndex = i;
					::Menu::activePageIndex = 0;
					::Menu::activeSubPageIndex = 0;
				}
			}

			const char* username = "FatalWidgets";
			const char* expireText = "by:";
			const char* expireDays = "addo6544.me";

			const auto usernameSize = fr->CalcTextSize(username) + ImVec2(0, ImGui::GetFontBaked()->Descent / 2);
			const auto expireTextSize = fr->CalcTextSize(expireText);
			const auto expireDaysSize = fr->CalcTextSize(expireDays);
			const auto spaceWidth = ImGui::CalcTextSize(" ").x;
			const auto lineWidthDiff = usernameSize.x - (expireTextSize.x + spaceWidth + expireDaysSize.x);

			ImGui::SetCursorPos(ImVec2(
				winRectSize.x - 20 - (30 + ImMax(usernameSize.x, expireTextSize.x + spaceWidth + expireDaysSize.x)),
				10
			));
			ImGui::BeginGroup();
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
				ImGui::BeginGroup();
				{
					ImGui::Dummy(ImVec2(0, (30 - (usernameSize.y + expireTextSize.y)) / 2));
					{
						if (lineWidthDiff < 0)
						{
							ImGui::Dummy(ImVec2(-lineWidthDiff, 0));
							ImGui::SameLine(0, 0);
						}
						ImGui::Dummy(usernameSize);
						fr->DrawText(titleDraws, username, ImGui::GetItemRectMin(), 0xFFFAFAFA);
					}
					{
						if (lineWidthDiff > 0)
						{
							ImGui::Dummy(ImVec2(lineWidthDiff, 0));
							ImGui::SameLine(0, 0);
						}
						ImGui::Dummy(expireTextSize + ImVec2(spaceWidth + expireDaysSize.x, 0));
						fr->DrawText(titleDraws, expireText, ImGui::GetItemRectMin(), 0xFF9E9E9E);
						fr->DrawText(titleDraws, expireDays, ImGui::GetItemRectMin() + ImVec2(expireTextSize.x + spaceWidth, 0), 0xFF6666FF);
					}
				}
				ImGui::EndGroup();
				ImGui::PopStyleVar();
				ImGui::SameLine(0, 4);
				{
					ImGui::Dummy(ImVec2(30, 30));
					titleDraws->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF666666, 15);
				}
			}
			ImGui::EndGroup();

		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		ImGui::Dummy(ImVec2(winRectSize.x, 1));
		draws->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF262626);

		draws->AddRectFilledMultiColor(
			ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2(winRectSize.x, 10),
			0x44000000, 0x44000000, 0, 0
		);
	}
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 14));
		ImGui::BeginChild("Content", ImVec2(winRectSize.x, 508), ImGuiChildFlags_AlwaysUseWindowPadding);
		{
			const Client::FeatureCategory& selectedCate = ::Menu::categories[::Menu::activeCateIndex];
			if (selectedCate.pages.size() > 1)
			{
				ImGui::BeginGroup();
				for (int i = 0; i < selectedCate.pages.size(); ++i)
				{
					const auto& page = selectedCate.pages[i];
					if (FatalWidgets::RadioButtonB(page.name, page.name, &::Menu::activePageIndex, i))
						::Menu::activeSubPageIndex = 0;
					ImGui::SameLine(0, 4);
				}
				ImGui::EndGroup();
				ImGui::Dummy(ImVec2(0, 6));
			}
			const Client::FeaturePage& selectedPage = selectedCate.pages[::Menu::activePageIndex];
			ImGui::BeginChild("Page", ImGui::GetContentRegionAvail());
			{
				if (selectedPage.subpages.size() > 1)
				{
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
					ImGui::BeginChild("SubPages", ImVec2(110, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_AlwaysUseWindowPadding);
					for (int i = 0; i < selectedPage.subpages.size(); ++i)
					{
						const auto& subPage = selectedPage.subpages[i];
						FatalWidgets::RadioButtonL(subPage.name, subPage.name, &::Menu::activeSubPageIndex, i);
					}
					ImGui::EndChild();
					ImGui::PopStyleVar(2);
				}
				const Client::FeatureSubPage& selectedSubPage = selectedPage.subpages[::Menu::activeSubPageIndex];

				ImGui::SameLine(0, 1);

				ImGui::BeginChild("Groups", ImGui::GetContentRegionAvail());
				const auto avail = ImGui::GetContentRegionAvail();
				const float groupWidth = (avail.x - 28) / 3;
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
				for (const auto& group : selectedSubPage.groups)
				{
					FatalWidgets::BeginGroup(group.name, ImVec2(groupWidth, group.height < 0 ? avail.y : group.height));
					for (int i = 0; i < group.features.size(); ++i)
					{
						ImGui::PushID(i);
						ImGui::BeginChild("Element", ImVec2(ImGui::GetContentRegionAvail().x, 24));
						{
							const auto& feature = group.features[i];
							ImGui::SetCursorPosY((24 - ImGui::GetTextLineHeight()) / 2);
							ImGui::Text("%s", feature.getName());
							float cursorX = ImGui::GetWindowSize().x;
							if (feature.hasProp())
							{
								if (const auto element = ::Menu::getPropElement(feature.getProp()))
								{
									ImGui::SameLine();
									const auto eSize = element->getSize();
									cursorX -= eSize.x;
									ImGui::SetCursorPos(ImVec2 { cursorX, (24 - eSize.y) / 2 });
									cursorX -= 6;
									element->render(feature.getProp());
								}
							}
							if (feature.hasSubProp())
							{
								ImGui::SameLine();
								cursorX -= 16;
								ImGui::SetCursorPos(ImVec2{ cursorX, 3.5 });
								if (ImGui::InvisibleButton("SubPropButton", ImVec2{ 16, 16 }))
									ImGui::OpenPopup("SubProps");
								ImGui::PushFont(IconFont);
								ImGui::GetWindowDrawList()->AddText(
									ImGui::GetItemRectMin(), 0xFF666666, reinterpret_cast<const char*>(u8"\ue665")
								);
								ImGui::PopFont();
								const bool subPropsOpen = ImGui::IsPopupOpen("SubProps");
								if (subPropsOpen)
								{
									ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 0));
									ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4);
									ImGui::PushStyleColor(ImGuiCol_Border, 0xFF222222);
									ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xFF1C1C1C);
									ImGui::SetNextWindowPos(ImGui::GetItemRectMin() + ImVec2(0, 22));
								}
								if (nova::ui::blur::BeginPopup("SubProps"))
								{
									const auto& subprops = feature.getSubProps();
									for (int subpropIndex = 0; subpropIndex < subprops.size(); ++subpropIndex)
									{
										const auto& subprop = subprops[subpropIndex];
										if (!subprop.hasProp() || !subprop.isVisible()) continue;
										const auto element = ::Menu::getPropElement(subprop.getProp());
										if (!element) continue;
										ImGui::PushID(subpropIndex);
										ImGui::BeginChild("Element", ImVec2(200, 24));
										{
											ImGui::SetCursorPosY((24 - ImGui::GetTextLineHeight()) / 2);
											ImGui::Text("%s", subprop.getName());
											ImGui::SameLine();
											const auto eSize = element->getSize();
											ImGui::SetCursorPos(ImVec2{ 200 - eSize.x, (24 - eSize.y) / 2 });
											element->render(subprop.getProp());
										}
										ImGui::EndChild();
										ImGui::PopID();
									}
									ImGui::EndPopup();
								}
								if (subPropsOpen)
								{
									ImGui::PopStyleColor(2);
									ImGui::PopStyleVar(2);
								}
							}
							ModuleContextMenu(feature);
						}
						ImGui::EndChild();
						ImGui::PopID();
					}
					FatalWidgets::EndGroup(group.name);
					ImGui::SameLine(0, 14);
				}
				ImGui::PopStyleVar();
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	{
		ImGui::Dummy(ImVec2(winRectSize.x, 1));
		draws->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF262626);
		draws->AddRectFilledMultiColor(
			ImGui::GetItemRectMin() + ImVec2(winRectSize.x, -10), ImGui::GetItemRectMin(),
			0, 0, 0x44000000, 0x44000000
		);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 0));
		ImGui::BeginChild("BottomBar", ImVec2(winRectSize.x, 28));

		const auto bottomDraws = ImGui::GetWindowDrawList();
		nova::ui::blur::LayerBackground(bottomDraws, ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), 0xFF1A1A1A, 6, ImDrawFlags_RoundCornersBottom);

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
}
