# logisth themes and window preferences

In SETTINGS / APPEARANCE, OPEN FOLDER reveals the user's `logisth/themes`
directory. The five standard palettes are written there on first use and can be
used as templates. The source and native packages also include them in
`assets/themes`. Missing standard files are recreated; existing files are not
silently overwritten.

The plug-in scans .json files every two seconds. RESCAN reads them immediately.
Files larger than 64 KiB, malformed JSON, unsupported formats/versions and invalid
colour definitions are ignored. Subfolders are not scanned. File names identify
selections, so installing a new file does not change the current selection.
Removing the selected file falls back to Midnight Blue.

A complete theme uses this format:

```json
{
  "format": "logisth-theme",
  "version": 1,
  "name": "Ocean",
  "dark": true,
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
    "screenText": "#E0EDFF"
  }
}
```

All ten roles are required. RGB values are six hexadecimal digits, optionally
prefixed by #. Name is 1–64 characters; dark is a JSON boolean controlling the
style of knobs, keys and buttons. Choose text/background colours with enough
contrast for your own display.

THEME EDITOR edits the selected palette as a draft. Choose a role, use the
colour picker, or type a hex value and press Enter. Changes preview immediately.
SAVE AS NEW writes a unique custom file and selects it; it does not overwrite a
standard or previously saved theme. Duplicate names receive a suffix such as (2).
Folder rescans retain unsaved draft colours when the selected saved file is unchanged. RESET EDITS restores the saved selection.
To revise an existing file in place, edit its JSON and use RESCAN (or wait for
the automatic scan). Theme changes propagate between open instances in the same
host process. Themes do not change synthesizer parameters or preset audio.

APPEARANCE also accepts 80–200% UI scaling, including two decimal places. The
percentage shown is the last accepted value. Preset percentage entries are
shortcuts; manual text accepts a decimal dot or comma. Invalid/out-of-range input
restores the last accepted value. Window resizing preserves the aspect ratio.
Actual pixel dimensions and scale are saved in the adjacent `appearance.json`.
A new/reopened editor restores the last size. Existing windows retain their own
sizes; closing an unchanged window does not overwrite a more recent resize in
another instance. Theme changes retain size, and audio presets retain both
appearance preferences. User preferences are separate from .logisth sound files.
