#include "Parameters.h"
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <pluginterfaces/base/ipluginbase.h>
int main(int argc, char **argv) {
    if (argc != 2)
        return 1;
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        std::cerr << dlerror() << '\n';
        return 1;
    }
    auto entry = reinterpret_cast<bool (*)(void *)>(dlsym(lib, "ModuleEntry"));
    auto exit = reinterpret_cast<bool (*)()>(dlsym(lib, "ModuleExit"));
    auto getFactory =
        reinterpret_cast<Steinberg::IPluginFactory *(*)()>(dlsym(lib, "GetPluginFactory"));
    if (!entry || !exit || !getFactory || !entry(lib))
        return 1;
    auto *factory = getFactory();
    if (!factory)
        return 1;
    Steinberg::PFactoryInfo manufacturer{};
    if (factory->getFactoryInfo(&manufacturer) != 0 ||
        std::strcmp(manufacturer.vendor, "wavelogik") != 0)
        return 1;
    bool found = false;
    for (int i = 0; i < factory->countClasses(); ++i) {
        Steinberg::PClassInfo info{};
        if (factory->getClassInfo(i, &info) == 0 &&
            std::strcmp(info.category, "Audio Module Class") == 0 &&
            std::strcmp(info.name, "logisth") == 0)
            found = true;
    }
    factory->release();
    exit();
    dlclose(lib);
    if (!found)
        return 1;
    std::cout << "PASS actual VST3: module entry and instrument factory discovery\n";
    return 0;
}
