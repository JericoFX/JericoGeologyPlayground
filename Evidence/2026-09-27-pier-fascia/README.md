# Curved pier fascia — 2026-09-27

The long side fascia previously connected the first and last pier frame with one straight beam. On a bowed canonical path it moved away from the deck edge. A regression fixture with two 1.2 m lateral offsets failed before the fix and passes now: each side stays within 0.25 m of the corresponding deck edge at 20% and 80% of arc length.

The fascia now uses the existing rectangular spline sweep, including bounded Bend/Collapsed, Loose/Detached and Broken variants. It retains the same semantic `Role::Fascia` ID and selectable part. Release `ALL_BUILD` passed, CTest passed 25/25, and the normal Vulkan assembly capture completed. `wooden-pier.png` shows the default shape after the fix.

The default pier remains visually basic; this geometric alignment fix does not establish final structural or material Beauty. The global shared-damage goal remains active.
