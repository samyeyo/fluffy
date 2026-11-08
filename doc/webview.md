# WebView

A `WebView` displays web content inside your window using the platform's native
web engine.

---

## What it's for

Use a `WebView` when you need to show a web page, a local HTML page, or raw
HTML inside your app and let the browser engine render it. It is useful for
documentation, embedded web apps, and rich HTML content that would be tedious to
rebuild from native widgets.

The web view is an overlay: the native page is composited on top of Fluffy's
own drawing. Until the native view is ready, a rounded placeholder appears in
its place. The web view can load a URL or HTML, run JavaScript, and receive JSON
messages from the page. Navigation and title callbacks are reserved for a future
native delegate.

---

## A first example

A window is the operating-system window your app draws into. `Window::open`
creates it, and `if (!win)` stops the program if the window could not be opened.
`win.add<T>(...)` creates a widget inside that window and returns a
reference to it. `Rect{x, y, width, height}` places the widget.

```cpp
#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("WebView demo", 720, 480);
    if (!win) return 1;

    auto& wv = win.add<fluffy::WebView>(
        fluffy::Rect{20, 20, 680, 320}, "https://example.com");

    win.run();
}
```

The constructor takes the bounds and an optional source URL, which defaults to
`https://example.com`. The native view is composited on top of the rest of your
Fluffy content.

The snippets below assume `win` and `wv` from this example.

---

## Declare in YAML

A `WebView` can also be created by the YAML loader:

```yaml
WebView:
  x: 20
  y: 20
  width: 680
  height: 320
  url: "https://example.com"
  radius: 12
```

The YAML keys are:

| Key | Meaning |
|---|---|
| `url` | URL to load. |
| `html` | Raw HTML to load instead of `url`. |
| `base_url` | Base URL for `html`, used to resolve relative assets. |
| `radius` | Corner radius in logical pixels, or `-1` to follow the theme. |

`baseUrl` is accepted as an alias for `base_url`; `corner_radius` and
`cornerRadius` are aliases for `radius`. Use one of `url` or `html`; when both
are present, the last one wins. For raw HTML:

```yaml
WebView:
  x: 20
  y: 20
  width: 680
  height: 320
  html: "<html><body><h1>Hello</h1></body></html>"
  base_url: "file:///Users/you/site/"
  radius: 12
```

YAML cannot attach typed callbacks. Add `setOnMessage`, `setOnNavigation`, and
`setOnTitle` in C++ after loading the UI.

---

## Common tasks

### Load content

```cpp
wv.navigate("https://www.google.com");
wv.navigate("file:///Users/you/site/index.html");
wv.setHtml("<html><body>Hello</body></html>");
wv.setHtml(html, "file:///Users/you/site/");
```

Use `navigate` to load a URL. Local files work with a `file://` URL, and
directories need a trailing `/`. Use `setHtml` for raw HTML. When the page
references relative assets or calls `fetch`, pass a base URL as the second
argument so those requests resolve.

### Run JavaScript

```cpp
wv.evalJS("document.title", [](const std::string& res) {
    std::printf("%s\n", res.c_str());
});
```

`evalJS` evaluates an expression in the page and delivers the result to your
callback as a string. Results are returned by the macOS, Windows, and Linux
backends. On WebAssembly there is no native web view, so `evalJS` answers
`"error: no native webview"`.

### Receive messages from the page

```cpp
wv.setOnMessage([](const std::string& json, fluffy::WebView& self) { });
```

`onMessage` receives messages posted by page JavaScript as JSON text. Fluffy
does not inject any helper object into the page. Instead, page code calls the
platform web view's own bridge:

```js
// macOS and Linux (WKWebView / WebKitGTK)
window.webkit.messageHandlers.fluffy.postMessage(JSON.stringify({type: 'buy', id: 42}));

// Windows (WebView2)
window.chrome.webview.postMessage(JSON.stringify({type: 'buy', id: 42}));
```

Because the bridge object belongs to the platform, the JavaScript call differs
by platform. Always pass JSON text with `JSON.stringify`; Fluffy delivers the
payload to `onMessage` as a string.

### React to navigation and title changes

```cpp
wv.setOnNavigation([](const std::string& url) { return true; });
wv.setOnTitle([](const std::string& title) { });
```

`onNavigation` lets you inspect a navigation request and return `false` to block
it. `onTitle` receives the page title. These callbacks are reserved for a future
native navigation delegate and are not delivered by the current backends.

### Control the corner radius

```cpp
wv.setCornerRadius(12.0f);
float r = wv.effectiveRadius(win.theme());
```

`setCornerRadius` fixes the radius in logical pixels, or uses `-1` to follow the
theme. A theme is the set of colors, fonts, and sizes your app uses.
`effectiveRadius` resolves the radius for the current theme.

---

## Good to know

- The `url` property is write-only: it is set by the constructor, `navigate`,
  `setHtml`, or the YAML `url`/`html` keys. There is no `url()` getter. The value
  is stored exactly as given; `setHtml` stores it with a `data:text/html,`
  prefix.
- On macOS, Windows, and Linux, `evalJS` returns the real result and `onMessage`
  receives page messages. On WebAssembly there is no native web view, so
  `evalJS` answers `"error: no native webview"` and no messages arrive.
- Windows uses WebView2's `NavigateToString` for `setHtml`, which has no base
  URL. Relative assets may not resolve there; prefer an absolute `file://` URL
  with `navigate` when local assets are needed. macOS and Linux apply the base
  URL passed to `setHtml`.
- Linux uses WebKitGTK and requires `webkit2gtk-4.1` plus an X11 window. Wayland
  windows fall back to the placeholder.
- A local `file://` page can read files in its containing folder. Pages loaded
  from `file://` still cannot fetch `https://` resources because of browser CORS
  rules, so keep local assets in the same folder as the page.
- The widget is focusable, takes focus when clicked, and reports its bounds as
  its natural size (falling back to `320`). Set an explicit width and height for
  a predictable overlay.
- Because this is a native overlay, it sits above Fluffy's canvas and is not
  clipped by Fluffy containers or scroll views. Keep its bounds inside the
  window.
- WebView support is enabled by default. Pass
  `-DFLUFFY_ENABLE_WEBVIEW=OFF` to compile it out and use the placeholder only.
  On Windows, CMake fetches the WebView2 package automatically; the WebView2
  Runtime must be installed. On Linux, install `libwebkit2gtk-4.1-dev` (or
  `libwebkit2gtk-4.0-dev`) and run on X11. The dependency is re-detected on
  every configure, so installing it later does not require clearing the cache.
  On WebAssembly only a placeholder is shown.

---

## Quick reference

### Properties

| Property | Type | Description |
|---|---|---|
| `url` | — | Source URL, write-only (constructor / `navigate` / `setHtml`). There is no `url()` getter. Stored verbatim (`setHtml` prefixes `data:text/html,`, no percent-encoding). |
| `cornerRadius` | `float` | `-1` tracks theme, otherwise fixed radius in logical pixels. |
| `effectiveRadius` | `float` | Resolved radius for the current theme (`cornerRadius < -0.5` tracks; re-applied only when the value changes). |

### Methods

```cpp
auto& wv = win.add<fluffy::WebView>(
    fluffy::Rect{20, 20, 680, 320}, "https://example.com");
wv.navigate("https://www.google.com");
wv.navigate("file:///Users/you/site/index.html");
wv.setHtml("<html><body>Hello</body></html>");
wv.setHtml(html, "file:///Users/you/site/");
wv.evalJS("document.title", [](const std::string& res) {
    std::printf("%s\n", res.c_str());
});

wv.setOnMessage([](const std::string& json, fluffy::WebView& self) { });
wv.setOnNavigation([](const std::string& url) { return true; });
wv.setOnTitle([](const std::string& title) { });

wv.setCornerRadius(12.0f);
float r = wv.effectiveRadius(win.theme());
```

### Events

| Event | Signature | When |
|---|---|---|
| `onMessage` | `void(const string& json, WebView&)` | Page calls `postMessage` (see JavaScript bridge). |
| `onNavigation` | `bool(const string& url)` | **Not delivered yet** (needs a native navigation delegate). |
| `onTitle` | `void(const string& title)` | **Not delivered yet** (needs a native navigation delegate). |

### YAML

```yaml
WebView:
  x: 20
  y: 20
  width: 680
  height: 320
  url: "https://example.com"
  radius: 12
```

```yaml
WebView:
  x: 20
  y: 20
  width: 680
  height: 320
  html: "<html><body><h1>Hello</h1></body></html>"
  base_url: "file:///Users/you/site/"
  radius: 12
```

| Key | Meaning |
|---|---|
| `url` | URL to load. |
| `html` | Raw HTML to load instead of `url`. |
| `base_url` | Base URL for `html`; resolves relative assets. |
| `radius` | Corner radius in logical pixels, or `-1` for the theme. |

Aliases: `baseUrl` for `base_url`; `corner_radius` and `cornerRadius` for
`radius`. If both `url` and `html` are present, the last key wins. Registered
`onXxx` names do not drive WebView callbacks; attach handlers in C++.

---

## See also

- [Window](window.md) — `nativeHandle()`.
