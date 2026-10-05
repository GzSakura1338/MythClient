#pragma once
#include <memory>
namespace nova::module { class Module; std::unique_ptr<Module> MakeAutoMaceModule(); }
namespace nova::module::automace {
bool IsWorking();
}
