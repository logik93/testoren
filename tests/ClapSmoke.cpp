// Minimal host: loads the actual CLAP binary, negotiates ports/parameters,
// sends a timed note and processes audio without an audio device.
#include "Parameters.h"
#include <array>
#include <clap/clap.h>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
#include <vector>
static void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct Events {
    clap_input_events_t api{};
    std::vector<clap_event_note_t> notes;
    Events() {
        api.ctx = this;
        api.size = [](const clap_input_events_t *p) -> uint32_t {
            return uint32_t(static_cast<Events *>(p->ctx)->notes.size());
        };
        api.get = [](const clap_input_events_t *p, uint32_t i) -> const clap_event_header_t * {
            return &static_cast<Events *>(p->ctx)->notes.at(i).header;
        };
    }
    void note(uint16_t type, uint32_t time) {
        clap_event_note_t n{};
        n.header = {sizeof(n), time, CLAP_CORE_EVENT_SPACE_ID, type, 0};
        n.note_id = -1;
        n.port_index = 0;
        n.channel = 0;
        n.key = 60;
        n.velocity = .8;
        notes.push_back(n);
    }
};
int main(int argc, char **argv) {
    try {
        check(argc == 2, "supply CLAP path");
        void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
        if (!lib)
            throw std::runtime_error(dlerror());
        auto *entry = static_cast<const clap_plugin_entry_t *>(dlsym(lib, "clap_entry"));
        check(entry && entry->init(argv[1]), "entry init");
        auto *factory =
            static_cast<const clap_plugin_factory_t *>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
        check(factory && factory->get_plugin_count(factory) == 1, "one instrument");
        clap_host_t host{};
        host.clap_version = CLAP_VERSION;
        host.name = "Logisth smoke host";
        host.vendor = "Testoren";
        host.url = "https://github.com/logik93/testoren";
        host.version = "1";
        host.get_extension = [](const clap_host_t *, const char *) -> const void * {
            return nullptr;
        };
        host.request_restart = [](const clap_host_t *) {};
        host.request_process = [](const clap_host_t *) {};
        host.request_callback = [](const clap_host_t *) {};
        auto *descriptor = factory->get_plugin_descriptor(factory, 0);
        check(std::strcmp(descriptor->id, "com.testoren.logisth") == 0, "plugin identity");
        auto *plugin = factory->create_plugin(factory, &host, descriptor->id);
        check(plugin && plugin->init(plugin), "plugin init");
        auto *ports = static_cast<const clap_plugin_audio_ports_t *>(
            plugin->get_extension(plugin, CLAP_EXT_AUDIO_PORTS));
        clap_audio_port_info_t port{};
        check(ports && ports->count(plugin, false) == 1 && ports->get(plugin, 0, false, &port) &&
                  port.channel_count == 2,
              "stereo output");
        auto *params = static_cast<const clap_plugin_params_t *>(
            plugin->get_extension(plugin, CLAP_EXT_PARAMS));
        check(params && params->count(plugin) == logisth::parameterCount, "all host parameters");
        auto *notePorts = static_cast<const clap_plugin_note_ports_t *>(
            plugin->get_extension(plugin, CLAP_EXT_NOTE_PORTS));
        check(notePorts && notePorts->count(plugin, false) > 0,
              "MIDI output port advertised to CLAP host");
        check(plugin->activate(plugin, 48000, 1, 512), "activate");
        check(plugin->start_processing(plugin), "start");
        std::array<float, 512> l{}, r{};
        float *channels[] = {l.data(), r.data()};
        clap_audio_buffer_t output{};
        output.data32 = channels;
        output.channel_count = 2;
        clap_output_events_t out{};
        out.try_push = [](const clap_output_events_t *, const clap_event_header_t *) -> bool {
            return true;
        };
        Events events;
        events.note(CLAP_EVENT_NOTE_ON, 128);
        clap_process_t process{};
        process.frames_count = 512;
        process.audio_outputs = &output;
        process.audio_outputs_count = 1;
        process.in_events = &events.api;
        process.out_events = &out;
        float maximum = 0;
        for (int b = 0; b < 200; ++b) {
            process.steady_time = int64_t(b) * 512;
            check(plugin->process(plugin, &process) != CLAP_PROCESS_ERROR, "process");
            for (int s = 0; s < 512; ++s) {
                check(std::isfinite(l[size_t(s)]) && std::abs(l[size_t(s)]) <= 1, "finite audio");
                if (b == 0 && s < 128)
                    check(l[size_t(s)] == 0, "event timing");
                maximum = std::max(maximum, std::abs(l[size_t(s)]));
            }
            events.notes.clear();
            if (b == 100)
                events.note(CLAP_EVENT_NOTE_OFF, 0);
        }
        check(maximum > .001f, "rendered note audio");
        plugin->stop_processing(plugin);
        plugin->deactivate(plugin);
        plugin->destroy(plugin);
        entry->deinit();
        dlclose(lib);
        std::cout << "PASS actual CLAP: discovery, stereo, all parameters, MIDI timing, audio\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
