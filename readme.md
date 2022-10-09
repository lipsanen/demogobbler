# demogobbler

demogobbler is a library for parsing and writing demo files written in C. Feature support:


| Game                | Basic packets | Net message parsing | Entity updates |
| --------------------- | :-------------: | :-------------------: | :--------------: |
| Aperture Tag        |      ✅      |         ✅         |       ✅       |
| BMS Retail          |      ✅      |         ❌         |       ❌       |
| CSS/TF2/HL2:DM      |      ✅      |         ✅         |       ✅       |
| HL2 2707 (OE)       |      ✅      |         ✅         |       ❌       |
| HL2 4044 (OE)       |      ✅      |         ❌         |       ❌       |
| HL2 steam           |      ✅      |         ✅         |       ✅       |
| HL:Source steam     |      ✅      |         ❌         |       ❌       |
| L4D1                |      ✅      |         ✅         |       ✅       |
| L4D2                |      ✅      |         ✅         |       ✅       |
| Portal 1 3420, 5135 |      ✅      |         ✅         |       ✅       |
| Portal 1 steam      |      ✅      |         ✅         |       ✅       |
| Portal 2            |      ✅      |         ✅         |       ✅       |
| Portal: Reloaded    |      ✅      |         ✅         |       ✅       |

# Building

1. Install meson
2. `meson setup build`
3. `meson compile -C build`

Linux and friends:
Install meson via your package manager.

Windows:
Meson can be installed through pip (the python package manager) via `pip install meson`. After this add the installation path of meson to your PATH. Steps 2 and 3 should be run through the Visual Studio Developer Command Prompt (Tools -> Command Line -> Developer Command Prompt).
