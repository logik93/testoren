#pragma once
#include "PluginEditor.h"
namespace theme_advanced_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool v, const char *text) {
        if (!v)
            throw std::runtime_error(text);
    };
    struct Profile {
        File previous = ThemeSettings::preferencesFile();
        TemporaryFile file{".json"};
        File root;
        Profile() {
            ThemeSettings::useStorageFileForTests(file.getFile());
            root = ThemeSettings::folder();
        }
        ~Profile() {
            ThemeSettings::useStorageFileForTests(previous);
            root.deleteRecursively();
            root.getSiblingFile(root.getFileName() + "-export").deleteRecursively();
        }
    } profile;
    uint32_t last = 0;
    check(!ThemeSettings::scanDue(119999, last) && ThemeSettings::scanDue(120000, last) &&
              !ThemeSettings::scanDue(239999, last) && ThemeSettings::scanDue(240000, last),
          "folder scan schedule is exactly two minutes");
    auto theme = ThemeSettings::theme(1);
    theme.name = "Portable custom";
    theme.fontFamily = Font::getDefaultMonospacedFontName();
    theme.backgroundStyle = 2;
    theme.knobStyle = 2;
    theme.colours[10] = Colour(0xffed5070);
    theme.colours[11] = Colour(0xff42536c);
    ThemeDefinition decoded;
    check(ThemeSettings::decode(ThemeSettings::encode(theme), decoded) &&
              decoded.fontFamily == theme.fontFamily && decoded.backgroundStyle == 2 &&
              decoded.knobStyle == 2 && decoded.colours == theme.colours,
          "version-2 theme codec retains font, background, knob style and every colour");
    auto directory = profile.root.getSiblingFile(profile.root.getFileName() + "-export");
    auto file = directory.getChildFile("Chosen filename.json");
    check(ThemeSettings::saveTo(theme, file).wasOk() && file.existsAsFile() &&
              ThemeSettings::folder() == directory,
          "theme Save As writes the exact chosen folder and file name");
    check(ThemeSettings::theme(ThemeSettings::reload()).name == theme.name &&
              ThemeSettings::folder() == directory,
          "selected custom theme folder survives preference reload");
    check(ThemeSettings::renameCurrent("Renamed custom").wasOk() && !file.existsAsFile(),
          "theme rename moves its file and retains a valid selection");
    auto saved = ThemeSettings::theme(ThemeSettings::current());
    check(saved.name == "Renamed custom" && saved.fontFamily == theme.fontFamily &&
              saved.colours == theme.colours,
          "renaming retains theme content");
    for (auto &stock : ThemeSettings::defaults())
        check(directory.getChildFile(stock.key + ".json").existsAsFile(),
              "chosen theme folder contains the five standard files");
    LogisthProcessor p;
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *console = ui->getChildComponent(0);
    dynamic_cast<Button *>(console->findChildWithID("ui.settings"))->onClick();
    auto *panel = console->findChildWithID("settings.panel");
    dynamic_cast<Button *>(panel->findChildWithID("settings.theme.edit"))->onClick();
    auto *hex = dynamic_cast<TextEditor *>(panel->findChildWithID("settings.theme.hex"));
    hex->setText("#FF8040", false);
    hex->onReturnKey();
    dynamic_cast<Button *>(panel->findChildWithID("settings.tab.2"))->onClick();
    auto *font = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.theme.font"));
    if (font->getNumItems() > 1)
        font->setSelectedId(2, sendNotificationSync);
    auto family = dynamic_cast<LogisthLook &>(ui->getLookAndFeel()).fontFamily;
    dynamic_cast<ComboBox *>(panel->findChildWithID("settings.theme.background"))
        ->setSelectedId(2, sendNotificationSync);
    dynamic_cast<ComboBox *>(panel->findChildWithID("settings.theme.knobs"))
        ->setSelectedId(2, sendNotificationSync);
    check(dynamic_cast<LogisthLook &>(ui->getLookAndFeel()).green == Colour(0xffff8040),
          "temporary colours preview live");
    ui.reset();
    check(p.appearanceDraft != nullptr, "closing the editor retains a per-instrument draft");
    ui.reset(p.createEditor());
    auto &look = dynamic_cast<LogisthLook &>(ui->getLookAndFeel());
    check(look.green == Colour(0xffff8040) && look.fontFamily == family &&
              look.backgroundStyle == 1 && look.knobStyle == 1,
          "recreated/minimized editor recalls temporary colours, font and styles");
    check(ThemeSettings::theme(ThemeSettings::current()).colours[5] == saved.colours[5],
          "temporary edits do not overwrite the saved file");
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    editor->snapshotTheme(0);
    check(look.green == Colour(0xffff8040), "preview snapshots preserve a live unsaved draft");
    console = ui->getChildComponent(0);
    panel = console->findChildWithID("settings.panel");
    dynamic_cast<TextEditor *>(panel->findChildWithID("settings.theme.name"))
        ->setText("Saved draft", false);
    dynamic_cast<Button *>(panel->findChildWithID("settings.theme.save"))->onClick();
    auto committed = ThemeSettings::theme(ThemeSettings::current());
    check(committed.name == "Saved draft" && committed.fontFamily == family &&
              committed.backgroundStyle == 1 && committed.knobStyle == 1 &&
              committed.colours[5] == Colour(0xffff8040),
          "saving commits every temporary style and colour to JSON");
    std::cout << "PASS advanced themes: chosen save path, rename, font/background/knobs, "
                 "two-minute scans, per-instance draft and editor recreation\n";
}
} // namespace theme_advanced_test
