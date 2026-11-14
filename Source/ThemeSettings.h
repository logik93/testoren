#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
// Appearance is a user preference, independent of the current sound/preset.
// Only GUI/control threads access this store; the audio callback never reads files or locks it.
namespace logisth {
class ThemeSettings {
  public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void themeChanged(int) = 0;
    };

  private:
    struct Store {
        juce::CriticalSection lock;
        juce::ListenerList<Listener> listeners;
        juce::File overrideFile;
        int selected = 1;
        bool loaded = false;
        std::atomic<uint64_t> revision{0};
        juce::File file() const {
            return overrideFile == juce::File{}
                       ? juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("logisth/appearance.json")
                       : overrideFile;
        }
        void read() {
            if (loaded)
                return;
            loaded = true;
            auto path = file();
            if (path.existsAsFile() && path.getSize() <= 4096) {
                auto value = juce::JSON::parse(path.loadFileAsString());
                if (auto *object = value.getDynamicObject()) {
                    auto theme = object->getProperty("theme");
                    if (theme.isInt() && int(theme) >= 0 && int(theme) < 5)
                        selected = int(theme);
                }
            }
        }
    };
    static Store &store() {
        static Store shared;
        return shared;
    }

  public:
    static int current() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        return s.selected;
    }
    static int reload() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        const int previous = s.selected;
        s.loaded = false;
        s.selected = 1;
        s.read();
        if (s.selected != previous)
            ++s.revision;
        return s.selected;
    }
    // Called on the message thread. Notify other open windows after releasing the file lock.
    static bool choose(int theme, Listener *origin = nullptr) {
        auto &s = store();
        bool saved = false;
        const int selected = juce::jlimit(0, 4, theme);
        {
            const juce::ScopedLock lock(s.lock);
            s.read();
            s.selected = selected;
            ++s.revision;
            auto path = s.file();
            if (path.getParentDirectory().createDirectory().wasOk()) {
                juce::TemporaryFile temp(path);
                auto text = "{\"theme\":" + juce::String(selected) + "}\n";
                saved =
                    temp.getFile().replaceWithText(text) && temp.overwriteTargetFileWithTemporary();
            }
        }
        s.listeners.call([&](Listener &listener) {
            if (&listener != origin)
                listener.themeChanged(selected);
        });
        return saved;
    }
    static void addListener(Listener *listener) {
        store().listeners.add(listener);
    }
    static void removeListener(Listener *listener) {
        store().listeners.remove(listener);
    }
    static uint64_t revision() {
        return store().revision.load();
    }
    // Isolate regression tests from the user's installed preferences.
    static void useStorageFileForTests(const juce::File &file) {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.overrideFile = file;
        s.loaded = false;
        s.selected = 1;
        ++s.revision;
    }
};
} // namespace logisth
