# Image

An `Image` shows a raster file (PNG, JPG, etc.) inside its `Rect` using the platform decoder.

![Image widget](../www/assets/widgets/image.png)

*A JPEG scaled to fit its bounds.*

## What it's for

Use an `Image` when you need to display raster graphics in a window. It loads either a `file` or raw `RGBA` pixels, uploads them as a texture, scales them to its `bounds` according to `scaleMode`, and can optionally draw a border.

## The words you need

- **raster image** — a picture made of pixels, such as a PNG or a JPG file.
- **RGBA** — the four bytes that describe one pixel: red, green, blue, alpha.
- **bounds** — where the image sits and how big it is, given as a `Rect`. See [Geometry & colors](geom.md).

In the examples below, `win` is an open window (see [Window](window.md)).

## A first example

```cpp
auto& img = win.add<fluffy::Image>(fluffy::Rect{40, 40, 300, 200}, "photo.png");
```

This creates an image inside the window and loads `photo.png`. The file is decoded by `CoreGraphics` on macOS, `WIC` on Windows, or `GdkPixbuf` on Linux. If the file cannot be read, the image stays empty; check it with `hasImage()`.

## Common tasks

### Load another file

```cpp
img.setImage("another.jpg");
```

The new file replaces the current image.

### Load raw RGBA pixels

```cpp
img.setImage(width, height, pixels); // RGBA (no-op on null pixels or non-positive size)
```

This overload takes the pixel dimensions and the RGBA data. It does nothing if `pixels` is null or the size is not positive.

### Check what is loaded

```cpp
bool ok = img.hasImage();
int w = img.imageWidth();
int h = img.imageHeight();
```

### Choose how the image fits its bounds

```cpp
img.setScaleMode(fluffy::ScaleMode::Fill);
fluffy::ScaleMode m = img.scaleMode();
```

- `Fit` fits the image inside the bounds, keeps the aspect ratio, and centers it.
- `Fill` covers the bounds, keeps the aspect ratio, and may clip the image.
- `Stretch` fills the bounds exactly and ignores the aspect ratio.
- `None` uses the native pixel size.

### Add a border or clear the image

```cpp
img.setBorder(true);
bool b = img.hasBorder();
img.clearImage();
```

## Good to know

- On WebAssembly there is no file decoder. `setImage(path)` is a stub, and only raw RGBA pixels display.
- `Image` has no YAML support because it is not registered with the YAML widget factory. Create it in code instead; see [YAML](yaml.md), where `Image` must be created in code.

The properties `file`, `scale_mode`, and `border` are available through `setProperty` for programmatic use, but not through YAML:

```yaml
# This would be ignored — Image not in factory
# Image:
#   file: "photo.png"
#   scale_mode: fit
#   border: true
```

## Quick reference

### Properties

| Property | Type | Description |
|---|---|---|
| `file` | `string` | Path to image file. Decoding via `CoreGraphics` (macOS) / `WIC` (Win) / `GdkPixbuf` (Linux). |
| `scaleMode` | `ScaleMode` | `Fit` (default) / `Fill` / `Stretch` / `None`. |
| `border` | `bool` | Whether to draw a `border` stroke. |

### Methods

```cpp
auto& img = win.add<fluffy::Image>(fluffy::Rect{40, 40, 300, 200}, "photo.png");
img.setImage("another.jpg");
img.setImage(width, height, pixels); // RGBA (no-op on null pixels or non-positive size)

bool ok = img.hasImage();
int w = img.imageWidth();
int h = img.imageHeight();

img.setScaleMode(fluffy::ScaleMode::Fill);
fluffy::ScaleMode m = img.scaleMode();

img.setBorder(true);
bool b = img.hasBorder();
img.clearImage();
```

### Scale modes

| `ScaleMode` | Effect |
|---|---|
| `Fit` | Fit inside, keep aspect, centered |
| `Fill` | Cover bounds, keep aspect, may clip |
| `Stretch` | Fill exactly, ignore aspect |
| `None` | Native pixels |

### Events

None.

## See also

- [Widget](widget.md)
