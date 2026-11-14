#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
// User appearance preferences; only GUI/control threads touch files or this store.
namespace logisth {
struct ThemeDefinition {
    juce::String key, name;
    bool dark = true, factory = false;
    std::array<juce::Colour, 12> colours{};
    juce::String fontFamily = "Open Sans";
    int backgroundStyle = 0, knobStyle = 0;
    bool same(const ThemeDefinition &t) const {
        return key == t.key && name == t.name && dark == t.dark && colours == t.colours &&
               fontFamily == t.fontFamily && backgroundStyle == t.backgroundStyle &&
               knobStyle == t.knobStyle;
    }
};
struct ThemeDraft {
    ThemeDefinition edited, baseline;
};
class ThemeSettings {
  public:
    static constexpr uint32_t scanIntervalMs = 120000;
    static bool scanDue(uint32_t now, uint32_t &last) {
        if (now - last < scanIntervalMs)
            return false;
        last = now;
        return true;
    }
    inline static const juce::StringArray colourNames{
        "body",      "panel",  "border",       "text",       "muted", "accent",
        "highlight", "screen", "screenBorder", "screenText", "knob",  "knobTrack"};
    struct Window {
        int width = 0, height = 0;
        double scale = 100;
    };
    struct Listener {
        virtual ~Listener() = default;
        virtual void themeChanged(int) = 0;
    };
    static std::vector<ThemeDefinition> defaults() {
        static constexpr juce::uint32 values[5][10] = {
            {0xffe8e9e2, 0xfff4f3ed, 0xffced2c9, 0xff283632, 0xff667268, 0xff406f5d, 0xffbdd8bb,
             0xff243430, 0xff41514b, 0xffe6ece1},
            {0xff0d1118, 0xff151d29, 0xff283a50, 0xffe0edff, 0xff9aadc5, 0xff62acff, 0xff91c8ff,
             0xff080d15, 0xff243952, 0xffe0edff},
            {0xff0c1725, 0xff11243a, 0xff28486a, 0xffdcf4ff, 0xff94b5cc, 0xff50c7ee, 0xff8de2ff,
             0xff071423, 0xff264862, 0xffdef6ff},
            {0xff111216, 0xff1c1e25, 0xff373c4b, 0xffe5e9f2, 0xffa4aab8, 0xff8aaaff, 0xffbbcaff,
             0xff0b0c10, 0xff343a49, 0xffe5e9f2},
            {0xff18201c, 0xff232d27, 0xff354238, 0xffe0e7d8, 0xff9aa99a, 0xffb0c9a5, 0xffbfd6b3,
             0xff101a15, 0xff35483b, 0xffe3eadc}};
        const char *keys[]{"00-light", "01-midnight-blue", "02-cobalt", "03-graphite", "04-forest"};
        const char *names[]{"Light", "Midnight Blue", "Cobalt", "Graphite", "Forest"};
        std::vector<ThemeDefinition> result;
        for (int i = 0; i < 5; ++i) {
            ThemeDefinition t;
            t.key = keys[i];
            t.name = names[i];
            t.dark = i != 0;
            t.factory = true;
            for (size_t c = 0; c < 10; ++c)
                t.colours[c] = juce::Colour(values[i][c]);
            t.colours[10] = t.colours[5];
            t.colours[11] = t.colours[2];
            result.push_back(t);
        }
        return result;
    }
    static juce::var encode(const ThemeDefinition &theme) {
        auto *o = new juce::DynamicObject;
        auto *colours = new juce::DynamicObject;
        o->setProperty("format", "logisth-theme");
        o->setProperty("version", 2);
        o->setProperty("font", theme.fontFamily);
        o->setProperty("backgroundStyle", theme.backgroundStyle);
        o->setProperty("knobStyle", theme.knobStyle);
        o->setProperty("name", theme.name);
        o->setProperty("dark", theme.dark);
        for (int i = 0; i < colourNames.size(); ++i)
            colours->setProperty(colourNames[i],
                                 "#" + theme.colours[size_t(i)].toDisplayString(false));
        o->setProperty("colours", juce::var(colours));
        return juce::var(o);
    }
    static bool colour(const juce::String &text, juce::Colour &result) {
        auto hex = text.trim().trimCharactersAtStart("#");
        if (hex.length() != 6 || !hex.containsOnly("0123456789abcdefABCDEF"))
            return false;
        result = juce::Colour::fromString("ff" + hex);
        return true;
    }
    static bool decode(const juce::var &value, ThemeDefinition &theme) {
        auto *o = value.getDynamicObject();
        if (!o)
            return false;
        if (o->getProperty("format").toString() != "logisth-theme" ||
            !o->getProperty("version").isInt() ||
            (int(o->getProperty("version")) != 1 && int(o->getProperty("version")) != 2))
            return false;
        auto name = o->getProperty("name");
        auto dark = o->getProperty("dark");
        if (!name.isString() || name.toString().trim().isEmpty() || name.toString().length() > 64 ||
            !dark.isBool())
            return false;
        auto *colours = o->getProperty("colours").getDynamicObject();
        if (!colours)
            return false;
        for (int i = 0; i < 10; ++i)
            if (!colour(colours->getProperty(colourNames[i]).toString(), theme.colours[size_t(i)]))
                return false;
        theme.colours[10] = theme.colours[5];
        theme.colours[11] = theme.colours[2];
        for (int i = 10; i < colourNames.size(); ++i)
            if (colours->hasProperty(colourNames[i]) &&
                !colour(colours->getProperty(colourNames[i]).toString(), theme.colours[size_t(i)]))
                return false;
        if (o->hasProperty("font")) {
            auto f = o->getProperty("font");
            if (!f.isString() || f.toString().trim().isEmpty() || f.toString().length() > 128)
                return false;
            theme.fontFamily = f.toString().trim();
        }
        for (auto property : {"backgroundStyle", "knobStyle"})
            if (o->hasProperty(property) &&
                (!o->getProperty(property).isInt() || int(o->getProperty(property)) < 0 ||
                 int(o->getProperty(property)) > 2))
                return false;
        theme.backgroundStyle = int(o->getProperty("backgroundStyle"));
        theme.knobStyle = int(o->getProperty("knobStyle"));
        theme.name = name.toString().trim();
        theme.dark = bool(dark);
        return true;
    }

  private:
    static bool writeJson(const juce::File &path, const juce::var &value) {
        if (path.getParentDirectory().createDirectory().failed())
            return false;
        juce::TemporaryFile temp(path);
        return temp.getFile().replaceWithText(juce::JSON::toString(value) + "\n") &&
               temp.overwriteTargetFileWithTemporary();
    }
    struct Store {
        juce::CriticalSection lock;
        juce::ListenerList<Listener> listeners;
        juce::File overrideFile, customFolder;
        std::vector<ThemeDefinition> themes = defaults();
        juce::String selectedKey = "01-midnight-blue", fileStamp, contentStamp;
        Window window;
        int selected = 1, ignored = 0;
        bool loaded = false;
        std::atomic<uint64_t> revision{0};
        juce::File file() const {
            return overrideFile == juce::File{}
                       ? juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("logisth/appearance.json")
                       : overrideFile;
        }
        juce::File folder() const {
            if (customFolder != juce::File{})
                return customFolder;
            return overrideFile == juce::File{}
                       ? file().getSiblingFile("themes")
                       : file().getSiblingFile(file().getFileNameWithoutExtension() + "-themes");
        }
        bool scan(bool force) {
            auto directory = folder();
            directory.createDirectory();
            auto standard = defaults();
            for (auto &t : standard) {
                auto f = directory.getChildFile(t.key + ".json");
                if (!f.exists())
                    writeJson(f, encode(t));
            }
            auto files = directory.findChildFiles(juce::File::findFiles, false, "*.json");
            files.sort();
            juce::String stamp;
            for (auto &f : files)
                stamp += f.getFileName() + ":" + juce::String(f.getSize()) + ":" +
                         juce::String(f.getLastModificationTime().toMilliseconds()) + ";";
            if (!force && stamp == fileStamp)
                return false;
            fileStamp = stamp;
            ignored = 0;
            auto next = standard;
            for (auto &f : files) {
                ThemeDefinition t;
                if (f.getSize() > 65536 || !decode(juce::JSON::parse(f.loadFileAsString()), t)) {
                    ++ignored;
                    continue;
                }
                t.key = f.getFileNameWithoutExtension();
                auto builtin = std::find_if(next.begin(), next.begin() + 5,
                                            [&](auto &v) { return v.key == t.key; });
                if (builtin != next.begin() + 5) {
                    t.factory = true;
                    *builtin = t;
                } else
                    next.push_back(t);
            }
            juce::String content;
            for (auto &t : next)
                content += t.key + juce::JSON::toString(encode(t), true);
            bool changed = content != contentStamp;
            contentStamp = content;
            themes = std::move(next);
            auto found = std::find_if(themes.begin(), themes.end(),
                                      [&](auto &t) { return t.key == selectedKey; });
            selected = found == themes.end() ? 1 : int(found - themes.begin());
            selectedKey = themes[size_t(selected)].key;
            if (changed)
                ++revision;
            return changed;
        }
        void read() {
            if (loaded)
                return;
            loaded = true;
            auto path = file();
            if (path.existsAsFile() && path.getSize() <= 8192) {
                auto value = juce::JSON::parse(path.loadFileAsString());
                if (auto *o = value.getDynamicObject()) {
                    auto path = o->getProperty("themeFolder").toString();
                    if (path.isNotEmpty() && juce::File::isAbsolutePath(path))
                        customFolder = juce::File(path);
                    auto old = o->getProperty("theme");
                    if (old.isInt() && int(old) >= 0 && int(old) < 5)
                        selectedKey = defaults()[size_t(int(old))].key;
                    auto key = o->getProperty("themeKey");
                    if (key.isString())
                        selectedKey = key.toString();
                    auto w = o->getProperty("width"), h = o->getProperty("height"),
                         scale = o->getProperty("scale");
                    if (w.isInt() && h.isInt() && int(w) >= 992 && int(w) <= 2480 &&
                        int(h) >= 688 && int(h) <= 1720 &&
                        std::abs(double(w) / double(h) - 1240. / 860.) < .005) {
                        window.width = int(w);
                        window.height = int(h);
                        window.scale = double(w) * 100 / 1240.;
                        if ((scale.isDouble() || scale.isInt()) && std::isfinite(double(scale)) &&
                            double(scale) >= 80 && double(scale) <= 200 &&
                            std::abs(double(scale) * 12.4 - int(w)) <= 1)
                            window.scale = double(scale);
                    }
                }
            }
            scan(true);
        }
        bool write() const {
            auto *o = new juce::DynamicObject;
            o->setProperty("themeFolder", customFolder == juce::File{}
                                              ? juce::String{}
                                              : customFolder.getFullPathName());
            o->setProperty("theme", selected);
            o->setProperty("themeKey", selectedKey);
            o->setProperty("width", window.width);
            o->setProperty("height", window.height);
            o->setProperty("scale", window.scale);
            return writeJson(file(), juce::var(o));
        }
    };
    static Store &store() {
        static Store s;
        return s;
    }
    static void notify(Listener *origin = nullptr) {
        auto &s = store();
        int selected = current();
        s.listeners.call([&](Listener &listener) {
            if (&listener != origin)
                listener.themeChanged(selected);
        });
    }

  public:
    static int current() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        return s.selected;
    }
    static ThemeDefinition theme(int id) {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        return s.themes[size_t(juce::jlimit(0, int(s.themes.size()) - 1, id))];
    }
    static juce::StringArray names() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        juce::StringArray result;
        for (auto &t : s.themes)
            result.add(t.name);
        return result;
    }
    static juce::File folder() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        return s.folder();
    }
    static juce::File preferencesFile() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        return s.file();
    }
    static int ignoredFiles() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        return s.ignored;
    }
    static int reload() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        const auto previous = s.selectedKey;
        s.loaded = false;
        s.selectedKey = "01-midnight-blue";
        s.window = {};
        s.read();
        if (previous != s.selectedKey)
            ++s.revision;
        return s.selected;
    }
    static bool scan(bool force = false) {
        auto &s = store();
        bool changed;
        {
            const juce::ScopedLock lock(s.lock);
            s.read();
            auto previous = s.selectedKey;
            changed = s.scan(force);
            if (s.selectedKey != previous)
                s.write();
        }
        if (changed)
            notify();
        return changed;
    }
    static bool choose(int id, Listener *origin = nullptr) {
        auto &s = store();
        bool saved;
        {
            const juce::ScopedLock lock(s.lock);
            s.read();
            s.selected = juce::jlimit(0, int(s.themes.size()) - 1, id);
            s.selectedKey = s.themes[size_t(s.selected)].key;
            ++s.revision;
            saved = s.write();
        }
        notify(origin);
        return saved;
    }
    static juce::Result setFolder(const juce::File &folder, Listener *origin = nullptr) {
        if (folder.createDirectory().failed())
            return juce::Result::fail("The theme folder could not be created.");
        auto &s = store();
        {
            const juce::ScopedLock lock(s.lock);
            s.read();
            s.customFolder = folder;
            s.fileStamp = {};
            s.scan(true);
            if (!s.write())
                return juce::Result::fail("The theme folder preference could not be saved.");
        }
        notify(origin);
        return juce::Result::ok();
    }
    static juce::Result renameCurrent(const juce::String &name, Listener *origin = nullptr,
                                      const ThemeDefinition *replacement = nullptr) {
        auto &s = store();
        {
            const juce::ScopedLock lock(s.lock);
            s.read();
            auto t = s.themes[size_t(s.selected)];
            if (t.factory)
                return juce::Result::fail("Use Save As for a standard theme.");
            if (replacement) {
                auto key = t.key;
                t = *replacement;
                t.key = key;
            }
            t.name = name.trim();
            ThemeDefinition checked;
            if (!decode(encode(t), checked))
                return juce::Result::fail("Use a theme name of 1-64 characters.");
            auto old = s.folder().getChildFile(t.key + ".json");
            auto next = s.folder().getNonexistentChildFile(
                "custom-" + juce::File::createLegalFileName(t.name), ".json", false);
            if (!writeJson(next, encode(t)))
                return juce::Result::fail("The renamed theme could not be saved.");
            if (!old.deleteFile()) {
                next.deleteFile();
                return juce::Result::fail("The previous theme file could not be renamed.");
            }
            s.selectedKey = next.getFileNameWithoutExtension();
            s.scan(true);
            s.write();
        }
        notify(origin);
        return juce::Result::ok();
    }
    static juce::Result saveTo(ThemeDefinition theme, const juce::File &file,
                               Listener *origin = nullptr) {
        ThemeDefinition checked;
        if (!file.hasFileExtension("json") || !decode(encode(theme), checked))
            return juce::Result::fail("Use a valid theme name and a .json file.");
        auto key = file.getFileNameWithoutExtension();
        for (auto &t : defaults())
            if (t.key.equalsIgnoreCase(key))
                return juce::Result::fail(
                    "Choose a custom file name to retain the standard theme.");
        auto directory = setFolder(file.getParentDirectory(), origin);
        if (directory.failed())
            return directory;
        auto &s = store();
        {
            const juce::ScopedLock lock(s.lock);
            s.read();
            if (!writeJson(file, encode(theme)))
                return juce::Result::fail("The theme file could not be saved.");
            s.selectedKey = key;
            s.scan(true);
            if (!s.write())
                return juce::Result::fail("Theme saved, but preferences could not be saved.");
        }
        notify(origin);
        return juce::Result::ok();
    }
    static juce::Result saveAs(ThemeDefinition theme, Listener *origin = nullptr) {
        theme.name = theme.name.trim();
        ThemeDefinition validated;
        if (!decode(encode(theme), validated))
            return juce::Result::fail("Use a theme name of 1-64 characters and valid colours.");
        auto &s = store();
        {
            const juce::ScopedLock lock(s.lock);
            s.read();
            const auto requested = theme.name;
            for (int copy = 2;
                 std::any_of(s.themes.begin(), s.themes.end(),
                             [&](auto &t) { return t.name.equalsIgnoreCase(theme.name); });
                 ++copy) {
                const auto suffix = " (" + juce::String(copy) + ")";
                theme.name = requested.substring(0, 64 - suffix.length()) + suffix;
            }
            auto stem = juce::File::createLegalFileName(theme.name).substring(0, 48);
            if (stem.isEmpty())
                stem = "Theme";
            auto file = s.folder().getNonexistentChildFile("custom-" + stem, ".json", false);
            if (!writeJson(file, encode(theme)))
                return juce::Result::fail("The theme file could not be saved.");
            s.selectedKey = file.getFileNameWithoutExtension();
            s.scan(true);
            if (!s.write())
                return juce::Result::fail(
                    "Theme saved, but the appearance preference could not be saved.");
        }
        notify(origin);
        return juce::Result::ok();
    }
    static Window window() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        return s.window;
    }
    static bool saveWindow(int width, int height, double scale) {
        if (width < 992 || width > 2480 || height < 688 || height > 1720 || !std::isfinite(scale) ||
            scale < 80 || scale > 200)
            return false;
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        s.window = {width, height, scale};
        return s.write();
    }
    static void addListener(Listener *l) {
        store().listeners.add(l);
    }
    static void removeListener(Listener *l) {
        store().listeners.remove(l);
    }
    static uint64_t revision() {
        return store().revision.load();
    }
    static void useStorageFileForTests(const juce::File &file) {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.overrideFile = file;
        s.customFolder = juce::File{};
        s.loaded = false;
        s.themes = defaults();
        s.selected = 1;
        s.selectedKey = "01-midnight-blue";
        s.window = {};
        s.fileStamp = {};
        s.contentStamp = {};
        ++s.revision;
    }
};
} // namespace logisth
