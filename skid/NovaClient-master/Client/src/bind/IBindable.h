#pragma once

namespace nova::bind {

class IBindable {
public:
    virtual ~IBindable() = default;

    [[nodiscard]] virtual int keybind() const = 0;
    virtual void onActive(int action) = 0;

    [[nodiscard]] virtual bool shouldActiveWhenRelease() const { return false; }
    [[nodiscard]] virtual bool isActive() const { return false; }
};

}
