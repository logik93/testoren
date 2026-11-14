# logisth themes and window preferences

SETTINGS / APPEARANCE / OPEN FOLDER reveals the current theme directory (initially
`logisth/themes` under your system user application-data directory). USE FOLDER
selects another directory and remembers it. Five standard palettes are written
there on first use; source/native packages include them in `assets/themes`.
Missing standard files are recreated; existing files are retained.

The plug-in polls .json files every **two minutes**. RESCAN reads them immediately.
Files larger than 64 KiB, malformed JSON, unsupported formats/versions and invalid
colours are ignored. Subfolders are not scanned. File names identify selections;
adding a theme does not switch the current selection. Removing the selected file
falls back to Midnight Blue.

Version 2 adds font, background, knob styles and two dedicated knob colours:

```json
{
  "format": "logisth-theme",
  "version": 2,
  "name": "Ocean",
  "dark": true,
  "font": "Open Sans",
  "backgroundStyle": 1,
  "knobStyle": 0,
  "colours": {
    "body": "#0D1118",
    "panel": "#151D29",
    "border": "#283A50",
    "text": "#E0EDFF",
    "muted": "#9AADC5",
    "accent": "#18C4D2",
    "highlight": "#91C8FF",
    "screen": "#080D15",
    "screenBorder": "#243952",
    "screenText": "#E0EDFF",
    "knob": "#18C4D2",
    "knobTrack": "#283A50"
  }
}
```

RGB values are six hexadecimal digits, optionally prefixed by #. Name is 1–64
characters; dark is a boolean controlling light/dark controls. Background styles
are 0 Flat, 1 Gradient, 2 Grid; knob styles are 0 Arc, 1 Solid, 2 Minimal. The
font is an installed family name (up to 128 characters). Open Sans is embedded;
other fonts must be installed on the destination machine and use system fallback
if unavailable. Font files are not embedded into a theme.

Version-1 files remain readable; missing extra knob colours default to accent
and border, font to Open Sans, and styles to Flat/Arc. All twelve palette roles
are emitted when saving version 2. Choose enough text/background contrast for
your display.

THEME EDITOR previews a draft: choose a role, use the colour picker or type a
hex value and press Enter. STYLE chooses font, background and knob drawing.
SAVE NEW writes a unique custom file in the current directory; duplicate names
receive a suffix. SAVE AS... chooses an exact folder/file name, and makes that
folder active. RENAME changes the selected custom theme's name/file and saves
the draft; standard files are protected. RESET restores the saved selection.

Unsaved colours and styles stay with the instrument instance when its editor
is destroyed/recreated or minimized. They are excluded from audio presets and
are not retained after destroying the instrument/closing the host: save a theme
for that. Folder rescans preserve a draft if its saved baseline is unchanged.
Selecting another saved theme replaces the draft. Saved selection changes
propagate between open instances in the same host process. Audio presets do not
change theme, scale or editor size.

APPEARANCE accepts 80–200% UI scaling, including decimals. The displayed value is
the last accepted percentage; manual input accepts a decimal dot or comma.
Invalid input restores the last accepted value. Resizing preserves the aspect
ratio. Actual pixel dimensions and scale are stored in `appearance.json` and
restored in new/reopened editors. Existing windows retain their own size.
