#include "plugin_api.h"
#include "plugin_manager.h"
#include <iostream>
#include <cassert>

int main() {
    PluginManager manager;
    bool loaded = manager.load_plugin("plugins/hello-world");
    assert(loaded);
    auto* api = manager.get_plugin_api("hello-world");
    assert(api);
    assert(api->name() == "hello-world");
    std::cout << "PluginAPI tests passed.\n";
    return 0;
}
