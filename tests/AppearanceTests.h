#pragma once
#include "PluginEditor.h"
#include <iostream>
#include <stdexcept>
namespace appearance_test {
using namespace juce;
using namespace logisth;
inline void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline void run() {
    struct Isolate {
        File previous = ThemeSettings::preferencesFile();
        TemporaryFile preference{".json"};
        File directory;
        Isolate() {
            ThemeSettings::useStorageFileForTests(preference.getFile());
            directory = ThemeSettings::folder();
        }
        ~Isolate() {
            ThemeSettings::useStorageFileForTests(previous);
            directory.deleteRecursively();
        }
    } isolation;
    check(ThemeSettings::names().size() == 5, "fresh user folder seeds all five default themes");
    for (const auto &definition : ThemeSettings::defaults()) {
        auto f = isolation.directory.getChildFile(definition.key + ".json");
        ThemeDefinition fromDisk;
        check(f.existsAsFile() &&
                  ThemeSettings::decode(JSON::parse(f.loadFileAsString()), fromDisk),
              "standard themes are valid editable JSON files");
        check(fromDisk.name == definition.name && fromDisk.colours == definition.colours,
              "factory JSON files retain every palette role");
    }
    auto custom = ThemeSettings::theme(1);
    custom.name = "Ocean / custom";
    custom.colours[5] = Colour(0xff12abef);
    check(ThemeSettings::saveAs(custom).wasOk(),
          "a custom theme is saved without replacing a factory file");
    check(ThemeSettings::names().size() == 6 &&
              ThemeSettings::theme(ThemeSettings::reload()).name == custom.name,
          "custom theme selection survives a preference reload");
    check(ThemeSettings::saveWindow(1705, 1183, 137.5),
          "fractional UI scale and exact dimensions persist");
    check(ThemeSettings::choose(2) && ThemeSettings::window().width == 1705,
          "theme changes retain the saved window size");
    {
        LogisthProcessor p;
        std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
        auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
        check(editor != nullptr, "appearance fixture editor");
        check(ui->getWidth() == 1705 && ui->getHeight() == 1183 && editor->uiScale() == 137.5,
              "fresh editor recalls exact dimensions and fractional scale");
        auto *console = ui->getChildComponent(0);
        auto *settings = dynamic_cast<Button *>(console->findChildWithID("ui.settings"));
        settings->onClick();
        auto *panel = console->findChildWithID("settings.panel");
        auto *scale = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.scale"));
        check(scale && scale->getText() == "137.5%",
              "Settings displays the last scale and allows typed values");
        {
            LogisthProcessor peer;
            std::unique_ptr<AudioProcessorEditor> peerUi(peer.createEditor());
            scale->setText("142.25%", sendNotificationSync);
            check(editor->uiScale() == 142.25 && ui->getWidth() == 1764 && ui->getHeight() == 1223,
                  "typed scaling resizes immediately and keeps the entered precision");
        }
        check(ThemeSettings::window().scale == 142.25,
              "closing an unchanged older instance does not overwrite the latest resized window");
        scale->setText("350%", sendNotificationSync);
        check(editor->uiScale() == 142.25 && scale->getText() == "142.25%",
              "out-of-range scale is rejected and last valid value is restored");
        scale->setText("bad", sendNotificationSync);
        check(scale->getText() == "142.25%", "invalid typed scale restores its last valid value");
        p.loadPreset(1);
        check(editor->uiScale() == 142.25, "loading audio presets keeps interface scale");
        auto *edit = dynamic_cast<Button *>(panel->findChildWithID("settings.theme.edit"));
        edit->onClick();
        auto *name = dynamic_cast<TextEditor *>(panel->findChildWithID("settings.theme.name"));
        auto *hex = dynamic_cast<TextEditor *>(panel->findChildWithID("settings.theme.hex"));
        auto *save = dynamic_cast<Button *>(panel->findChildWithID("settings.theme.save"));
        check(name && hex && save && name->isVisible(),
              "theme editor stays inside the settings panel");
        name->setText("Handmade Ocean", false);
        hex->setText("#18C4D2", false);
        hex->onReturnKey();
        check(dynamic_cast<LogisthLook &>(ui->getLookAndFeel()).green == Colour(0xff18c4d2),
              "manual colour edits preview the actual plugin palette");
        save->onClick();
        check(ThemeSettings::theme(ThemeSettings::current()).name == "Handmade Ocean" &&
                  ThemeSettings::theme(ThemeSettings::current()).colours[5] == Colour(0xff18c4d2),
              "GUI saves edited theme colours and selects the new file");
        hex->setText("#FF8040", false);
        hex->onReturnKey();
        auto activeKey = ThemeSettings::theme(ThemeSettings::current()).key;
        auto external = custom;
        external.name = "A newly installed theme";
        auto externalFile = isolation.directory.getChildFile("custom-00-external.json");
        check(externalFile.replaceWithText(JSON::toString(ThemeSettings::encode(external))),
              "external theme fixture saved");
        Thread::sleep(2100);
        Timer::callPendingTimersSynchronously();
        auto *themes = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.theme"));
        check(themes->getNumItems() == 7, "theme folder is not polled after only two seconds");
        dynamic_cast<Button *>(panel->findChildWithID("settings.theme.scan"))->onClick();
        check(themes->getNumItems() == 8 &&
                  ThemeSettings::theme(ThemeSettings::current()).key == activeKey,
              "manual folder scanning discovers installed themes without changing stable "
              "selection");
        check(dynamic_cast<LogisthLook &>(ui->getLookAndFeel()).green == Colour(0xffff8040),
              "folder rescans preserve an unsaved palette draft when the saved selection is "
              "unchanged");
        dynamic_cast<Button *>(panel->findChildWithID("settings.theme.reset"))->onClick();
        check(dynamic_cast<LogisthLook &>(ui->getLookAndFeel()).green == Colour(0xff18c4d2),
              "Reset Edits restores saved colours after a catalogue change");
        auto revision = ThemeSettings::revision();
        check(!ThemeSettings::scan(true) && revision == ThemeSettings::revision(),
              "unchanged rescans do not rebuild menus or flicker controls");
        for (int tab = 0; tab < 4; ++tab) {
            dynamic_cast<Button *>(panel->findChildWithID("settings.tab." + String(tab)))
                ->onClick();
            auto children = panel->getChildren();
            for (int i = 0; i < children.size(); ++i)
                if (children[i]->isVisible()) {
                    check(panel->getLocalBounds().contains(children[i]->getBounds()),
                          "settings controls stay inside their panel");
                    for (int j = 0; j < i; ++j)
                        if (children[j]->isVisible())
                            check(!children[i]->getBounds().intersects(children[j]->getBounds()),
                                  "settings controls do not overlap");
                }
        }
        isolation.directory.getChildFile("broken.json").replaceWithText("{invalid");
        auto bad = ThemeSettings::encode(custom);
        bad.getDynamicObject()->getProperty("colours").getDynamicObject()->setProperty(
            "accent", "not-a-colour");
        isolation.directory.getChildFile("invalid-colour.json")
            .replaceWithText(JSON::toString(bad));
        ThemeSettings::scan(true);
        check(ThemeSettings::names().size() == 8 && ThemeSettings::ignoredFiles() == 2,
              "malformed files and invalid colours are ignored without losing valid themes");
        ui->setSize(1550, 1075);
    }
    {
        LogisthProcessor p;
        std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
        check(ui->getWidth() == 1550 && ui->getHeight() == 1075,
              "closing after a host resize saves the exact last size");
        check(dynamic_cast<LogisthEditor *>(ui.get())->uiScale() == 125,
              "dragged size appears as the corresponding scale on reopen");
        check(ThemeSettings::theme(ThemeSettings::current()).name == "Handmade Ocean",
              "custom theme persists independently of sound and window size");
    }
    auto duplicate = ThemeSettings::theme(ThemeSettings::current());
    check(ThemeSettings::saveAs(duplicate).wasOk() &&
              ThemeSettings::theme(ThemeSettings::current()).name == "Handmade Ocean (2)",
          "saving a copy with an existing name adds a readable suffix without overwriting a file");
    std::cout << "PASS appearance: persisted size, typed scale, isolated instances, five JSON "
                 "defaults, custom editor, folder scanning and stable selection\n";
}
inline void preview(const File &folder) {
    folder.createDirectory();
    LogisthProcessor p;
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    check(editor != nullptr, "preview editor");
    editor->setUIScale(100);
    auto write = [&](const String &name, const Image &image) {
        FileOutputStream out(folder.getChildFile(name + ".png"));
        out.setPosition(0);
        out.truncate();
        PNGImageFormat png;
        check(png.writeImageToStream(image, out), "feature preview PNG export");
    };
    auto *console = ui->getChildComponent(0);
    dynamic_cast<Button *>(console->findChildWithID("ui.settings"))->onClick();
    auto *panel = console->findChildWithID("settings.panel");
    for (int tab = 0; tab < 4; ++tab) {
        dynamic_cast<Button *>(panel->findChildWithID("settings.tab." + String(tab)))->onClick();
        write("settings-" + String(tab), ui->createComponentSnapshot(ui->getLocalBounds()));
    }
    for (int theme = 0; theme < 5; ++theme)
        write("theme-" + String(theme), editor->snapshotTheme(theme));
    for (int type = 0; type < 5; ++type)
        write("reverb-" + String(type), editor->snapshotPage(14 + type));
    for (int page = 19; page <= 22; ++page)
        write("feature-" + String(page), editor->snapshotPage(page));
}
} // namespace appearance_test
