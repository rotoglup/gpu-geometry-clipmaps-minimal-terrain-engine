After browsing the web for a usable terrain lib, I ended up writing a new one as the others were too bloated.

The result is a minimal implementation of the original of Geometry ClipMaps: Terrain rendering using nested regular grids in only 200 lines. Performance is around 1000-2000 fps.

Key settings:
Space : wireframe view
Enter : Top-down view

Original Siggraph Paper:

Geometry clipmaps: Terrain rendering using nested regular grids,
Frank Losasso, Hugues Hoppe.
ACM Trans. Graphics (SIGGRAPH), 23(3), 2004.
New terrain data structure enabling real-time decompression and synthesis.