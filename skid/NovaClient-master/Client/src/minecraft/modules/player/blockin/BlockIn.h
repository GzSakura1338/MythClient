#pragma once
namespace nova::module::blockin {
// Input ownership is published on the client thread; hooks only read it.
bool IsPlacing();
}
