#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "utils/Color.h"
#include "utils/Json.h"

namespace Properties
{
	using Json = nova::json::Value;

	class AbstractProperty
	{
	public:
		virtual ~AbstractProperty() = default;

		virtual void save(Json& out) const = 0;
		virtual void load(const Json& in) = 0;
	};

	class PropDesc;

	template <typename T>
	class PropBase : public AbstractProperty
	{
		T value;
	protected:
		virtual T processValue(T valIn)
		{
			return valIn;
		}
		explicit PropBase(T defaultValue)
			: AbstractProperty(), value(defaultValue)
		{}
	public:
		void set(T newVal)
		{
			value = processValue(newVal);
		}
		[[nodiscard]]
		T get() const
		{
			return value;
		}
	};
	class NumberProp : public PropBase<double>
	{
		double min;
		double max;
		double step;
		std::function<std::string(double)> formatFn;
	protected:
		double processValue(double valIn) override
		{
			return processEx(valIn, min, max, step);
		}
	public:
		static double processEx(double in, double mn, double mx, double step)
		{
			return std::clamp(std::round(in / step) * step, mn, mx);
		}

		double getMin() const
		{
			return min;
		}
		double getMax() const
		{
			return max;
		}
		double getStep() const
		{
			return step;
		}

		std::string formatValue(double val) const
		{
			return formatFn(val);
		}

		void save(Json& out) const override
		{
			out = Json(get());
		}

		void load(const Json& in) override
		{
			if (in.isNumber())
				set(in.asNumber(get()));
		}

		NumberProp(double minIn, double maxIn, double stepIn, double defaultValue, std::function<std::string(double)>&& formatFnIn)
			: PropBase(processEx(defaultValue, minIn, maxIn, stepIn)), min(minIn), max(maxIn), step(stepIn), formatFn(std::move(formatFnIn))
		{}
		NumberProp(double minIn, double maxIn, double stepIn, double defaultValue, const char* formatIn = "{}")
			: NumberProp(minIn, maxIn, stepIn, defaultValue, [formatIn](double val) { return std::vformat(formatIn, std::make_format_args(val)); })
		{}
	};

	class BoolProperty : public PropBase<bool>
	{
	public:
		BoolProperty(bool defaultValue)
			: PropBase(defaultValue)
		{}

		void save(Json& out) const override
		{
			out = Json(get());
		}

		void load(const Json& in) override
		{
			if (in.isBool())
				set(in.asBool(get()));
		}
	};

	class StringProperty : public PropBase<std::string>
	{
	public:
		explicit StringProperty(std::string value = {}) : PropBase(std::move(value)) {}
		void save(Json& out) const override { out = get(); }
		void load(const Json& in) override { if (in.isString()) set(in.asString()); }
	};

	class ModeProperty : public PropBase<int>
	{
		std::vector<std::string> values;
	protected:
		int processValue(int valIn) override
		{
			if (values.empty())
				return 0;
			return std::clamp(valIn, 0, static_cast<int>(values.size()) - 1);
		}
	public:
		ModeProperty(const int defaultValueIndex, std::vector<std::string>&& valuesIn)
			: PropBase(0), values(std::move(valuesIn))
		{
			set(defaultValueIndex);
		}

		void save(Json& out) const override
		{
			const int index = get();
			const bool valid = index >= 0 && index < static_cast<int>(values.size());
			out = Json(valid ? values[static_cast<std::size_t>(index)] : std::string{});
		}

		void load(const Json& in) override
		{
			if (in.isString()) {
				const std::string name = in.asString();
				for (std::size_t i = 0; i < values.size(); ++i) {
					if (values[i] == name) {
						set(static_cast<int>(i));
						return;
					}
				}
				return;
			}
			if (in.isNumber())
				set(static_cast<int>(in.asNumber(get())));
		}

		std::string_view getValueName(int value) const
		{
			return values[value];
		}

		[[nodiscard]]
		const char* getValueNameCStr(int value) const
		{
			return values[value].c_str();
		}

		[[nodiscard]]
		int getValueCount() const
		{
			return static_cast<int>(values.size());
		}
	};

	class ColorProperty : public AbstractProperty
	{
		float value[4];
		bool alphaEnabled;
	public:
		ColorProperty(std::uint32_t colorAbgr, bool allowsAlpha)
			: alphaEnabled(allowsAlpha)
		{
			nova::color::BgrToHsb(colorAbgr, value);
			value[3] = allowsAlpha
				? static_cast<float>(nova::color::Alpha(colorAbgr)) / 255.0f
				: -1.0f;
		}

		[[nodiscard]] bool allowsAlpha() const
		{
			return alphaEnabled;
		}

		[[nodiscard]] const float* hsba() const
		{
			return value;
		}

		void setHsba(const float in[4])
		{
			setH(in[0]);
			setS(in[1]);
			setB(in[2]);
			setA(in[3]);
		}

		[[nodiscard]] float h() const { return value[0]; }
		[[nodiscard]] float s() const { return value[1]; }
		[[nodiscard]] float b() const { return value[2]; }
		[[nodiscard]] float a() const { return value[3]; }

		void setH(float in) { value[0] = nova::color::Clamp(in, 0.0f, 1.0f); }
		void setS(float in) { value[1] = nova::color::Clamp(in, 0.0f, 1.0f); }
		void setB(float in) { value[2] = nova::color::Clamp(in, 0.0f, 1.0f); }
		void setA(float in)
		{
			if (!alphaEnabled) return;
			value[3] = nova::color::Clamp(in, 0.0f, 1.0f);
		}

		[[nodiscard]]
		std::uint32_t valueAbgr() const
		{
			std::uint32_t packed = nova::color::HsbToBgr(h(), s(), b());
			if (alphaEnabled)
				packed = nova::color::ReAlpha(packed, static_cast<int>(a() * 255.0f));
			return packed;
		}

		void save(Json& out) const override
		{
			out = Json::Object();
			out["h"] = static_cast<double>(h());
			out["s"] = static_cast<double>(s());
			out["b"] = static_cast<double>(b());
			if (alphaEnabled)
				out["a"] = static_cast<double>(a());
		}

		void load(const Json& in) override
		{
			if (!in.isObject())
				return;
			if (const Json* value = in.Find("h")) setH(static_cast<float>(value->asNumber(h())));
			if (const Json* value = in.Find("s")) setS(static_cast<float>(value->asNumber(s())));
			if (const Json* value = in.Find("b")) setB(static_cast<float>(value->asNumber(b())));
			if (const Json* value = in.Find("a")) setA(static_cast<float>(value->asNumber(a())));
		}
	};

	class ColorProp : public AbstractProperty
	{
	public:
		enum class Mode : int
		{
			Static = 0,
			Rainbow,
			Gradient,
		};

	private:
		ModeProperty mode;
		NumberProp speed;
		NumberProp range;
		NumberProp rainbowSaturation;
		NumberProp rainbowBrightness;
		NumberProp gradientColorCount;
		std::array<ColorProperty, 7> colors;

	public:
		ColorProp(std::uint32_t defaultColor, bool allowsAlpha)
			: mode(static_cast<int>(Mode::Static), {"Static", "Rainbow", "Gradient"})
			, speed(1, 10, 1, 4, "{:.0f}")
			, range(1, 50, 1, 25, "{:.0f}")
			, rainbowSaturation(0, 1, 0.01, 0.8, "{:.2f}")
			, rainbowBrightness(0, 1, 0.01, 1, "{:.2f}")
			, gradientColorCount(2, 6, 1, 2, "{:.0f}")
			, colors{ ColorProperty(defaultColor, allowsAlpha),
			          ColorProperty(0xFFFFFFFFu, allowsAlpha),
			          ColorProperty(0xFFFFFFFFu, allowsAlpha),
			          ColorProperty(0xFFFFFFFFu, allowsAlpha),
			          ColorProperty(0xFFFFFFFFu, allowsAlpha),
			          ColorProperty(0xFFFFFFFFu, allowsAlpha),
			          ColorProperty(0xFFFFFFFFu, allowsAlpha) }
		{}

		[[nodiscard]]
		Mode getMode() const
		{
			return static_cast<Mode>(mode.get());
		}

		[[nodiscard]]
		bool is(Mode candidate) const
		{
			return getMode() == candidate;
		}

		[[nodiscard]] NumberProp& speedProp() { return speed; }
		[[nodiscard]] NumberProp& rangeProp() { return range; }
		[[nodiscard]] NumberProp& rainbowSaturationProp() { return rainbowSaturation; }
		[[nodiscard]] NumberProp& rainbowBrightnessProp() { return rainbowBrightness; }
		[[nodiscard]] NumberProp& gradientColorCountProp() { return gradientColorCount; }

		[[nodiscard]] ColorProperty& color(int slot)
		{
			return colors[static_cast<std::size_t>(std::clamp(slot, 0, 6))];
		}

		[[nodiscard]]
		std::uint32_t getColorAbgr(int index = 0) const
		{
			if (mode.get() != static_cast<int>(Mode::Static))
			{
				const int rangeValue = static_cast<int>(range.get());
				const int speedValue = static_cast<int>(speed.getMax()) +
				                       static_cast<int>(speed.getMin()) -
				                       static_cast<int>(speed.get());

				std::array<std::uint32_t, 6> palette{};
				std::size_t count = 0;
				if (mode.get() == static_cast<int>(Mode::Gradient))
				{
					count = static_cast<std::size_t>(std::clamp(static_cast<int>(gradientColorCount.get()), 2, 6));
					for (std::size_t i = 0; i < count; ++i)
						palette[i] = colors[i + 1].valueAbgr();
				}
				else
				{
					constexpr std::size_t kStops = 6;
					constexpr float kHueStep = 1.0f / 7.0f;
					const float saturation = static_cast<float>(rainbowSaturation.get());
					const float brightness = static_cast<float>(rainbowBrightness.get());
					for (std::size_t i = 0; i < kStops; ++i)
						palette[i] = nova::color::HsbToBgr(kHueStep * static_cast<float>(i), saturation, brightness);
					count = kStops;
				}

				return nova::color::MultiGradient(
					index, speedValue, rangeValue,
					std::span<const std::uint32_t>(palette.data(), count));
			}
			return colors[0].valueAbgr();
		}

		void save(Json& out) const override
		{
			out = Json::Object();
			out["mode"] = mode.get();
			out["speed"] = speed.get();
			out["range"] = range.get();
			out["rainbowSaturation"] = rainbowSaturation.get();
			out["rainbowBrightness"] = rainbowBrightness.get();
			out["gradientColorCount"] = gradientColorCount.get();

			Json list = Json::Array();
			for (const ColorProperty& entry : colors) {
				Json item;
				entry.save(item);
				list.Push(std::move(item));
			}
			out["colors"] = std::move(list);
		}

		void load(const Json& in) override
		{
			if (!in.isObject())
				return;
			if (const Json* value = in.Find("mode")) mode.load(*value);
			if (const Json* value = in.Find("speed")) speed.load(*value);
			if (const Json* value = in.Find("range")) range.load(*value);
			if (const Json* value = in.Find("rainbowSaturation")) rainbowSaturation.load(*value);
			if (const Json* value = in.Find("rainbowBrightness")) rainbowBrightness.load(*value);
			if (const Json* value = in.Find("gradientColorCount")) gradientColorCount.load(*value);

			const Json* list = in.Find("colors");
			if (!list || !list->isArray())
				return;
			const std::vector<Json>& items = list->Items();
			for (std::size_t i = 0; i < items.size() && i < colors.size(); ++i)
				colors[i].load(items[i]);
		}

		[[nodiscard]]
		std::vector<PropDesc> descriptors();
	};

	class PropDesc
	{
	public:
		using Visibility = std::function<bool()>;

	private:
		const char* name;
		AbstractProperty* prop;
		std::vector<PropDesc> subProps;
		Visibility visible;
	public:
		[[nodiscard]]
		bool hasProp() const
		{
			return prop != nullptr;
		}
		[[nodiscard]]
		bool hasSubProp() const
		{
			return !subProps.empty();
		}
		[[nodiscard]]
		bool isVisible() const
		{
			return !visible || visible();
		}

		AbstractProperty* getProp() const
		{
			return prop;
		}
		const std::vector<PropDesc>& getSubProps() const
		{
			return subProps;
		}

		const char* getName() const
		{
			return name;
		}

		PropDesc(const char* nameIn, AbstractProperty* propIn, std::vector<PropDesc>&& subPropsIn = {},
		         Visibility visibleIn = {})
			: name(nameIn), prop(propIn), subProps(std::move(subPropsIn)), visible(std::move(visibleIn))
		{}
	};

	inline std::vector<PropDesc> ColorProp::descriptors()
	{
		ColorProp* self = this;

		const auto row = [self](auto* property, const char* label, bool (*visible)(Mode)) {
			return PropDesc(label, property, {}, [self, visible] { return visible(self->getMode()); });
		};

		std::vector<PropDesc> rows;
		rows.reserve(13);
		rows.emplace_back("Type", &mode);
		rows.push_back(row(&speed, "Speed", [](Mode m) { return m != Mode::Static; }));
		rows.push_back(row(&range, "Range", [](Mode m) { return m != Mode::Static; }));
		rows.push_back(row(&rainbowSaturation, "Rainbow Saturation",
		                   [](Mode m) { return m == Mode::Rainbow; }));
		rows.push_back(row(&rainbowBrightness, "Rainbow Brightness",
		                   [](Mode m) { return m == Mode::Rainbow; }));
		rows.push_back(row(&gradientColorCount, "Gradient Colors",
		                   [](Mode m) { return m == Mode::Gradient; }));
		rows.emplace_back("Static Color", &colors[0], std::vector<PropDesc>{},
		                  [self] { return self->is(Mode::Static); });

		static constexpr const char* kGradientNames[6] = {
			"Gradient Color 1", "Gradient Color 2", "Gradient Color 3",
			"Gradient Color 4", "Gradient Color 5", "Gradient Color 6",
		};
		for (int slot = 1; slot <= 6; ++slot)
		{
			rows.emplace_back(kGradientNames[slot - 1], &colors[slot], std::vector<PropDesc>{},
			                  [self, slot] {
				                  return self->is(Mode::Gradient) &&
				                         static_cast<int>(self->gradientColorCount.get()) >= slot;
			                  });
		}
		return rows;
	}
}
