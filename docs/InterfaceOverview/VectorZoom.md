# UI Zoom

UI Zoom magnifies the entire application window — sidebar, toolbar, lists, and content pages together — as a single unit. It is designed for screen sharing and presentations, where a larger, more legible UI helps viewers on the call follow along.

Unlike zooming a picture, UI Zoom is a true vector re-layout: the whole interface reflows at the new size rather than being scaled up as a bitmap. Text stays crisp and sharp at every zoom level, and layouts (columns, wrapped text, list rows) rearrange to fit the new effective window size instead of just appearing blurrier or cropped.

## Zooming In and Out

**To zoom with the keyboard:**

1. Press **Ctrl** and **+** (or **Ctrl** and **=**) to zoom in.
2. Press **Ctrl** and **-** to zoom out.
3. Press **Ctrl** and **0** to reset zoom to 100%.



**To zoom with the mouse:**

1. Hold **Ctrl** and roll the mouse wheel up to zoom in, or down to zoom out.

The same commands are also available from the [Application Menu](<ApplicationMenu.md>)'s **View** group, as a single **−  /  percentage  /  +** row.

Each step changes the zoom level by 10%. Zoom is clamped between 70% and 250%, so you cannot zoom out or in past a point where the interface would become unusable.

## Full Screen

The same **View** group row also has a fullscreen button, which hides the window's title bar and OS chrome so the interface fills the entire screen — useful together with UI Zoom for presentations.

**To toggle full screen:**

1. Click the fullscreen icon at the right end of the zoom row, or press **F11** (Windows/Linux) / **Control+Command+F** (macOS).

**To leave full screen:** press **Esc**, or use the same button/shortcut again. Esc only ever exits full screen — it won't enter it — and only acts once nothing else on screen (an open dialog, a note's Find and Replace bar) is already using Esc for something of its own.

## Notes on Screen Sharing

Because zoom reflows the layout rather than scaling a rendered image, the interface remains readable at high zoom levels instead of turning pixelated the way a screenshot or video stream scaled up would. This makes it well suited to live demonstrations where the audience is viewing a shared screen at reduced size or resolution.

Zoom applies to the main application window. Popup windows that render as separate floating windows — such as the application menu or a spell-check suggestion menu — are drawn at their own native size and are not affected by the current zoom level.
