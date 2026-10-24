# Tutorial 9 — WebView

Show web content inside a Fluffy window, with corners that follow the current theme.

## What you will build

A window containing a `WebView` that loads `https://example.com`, prints messages
sent by the page, and keeps its rounded corners in sync with the theme. The step
by step also shows how to load your own HTML and how the navigation callbacks are
reserved for a future native delegate.

## What you will learn

- What a `WebView` is and how to place it in a window.
- How to load a URL with `navigate`.
- How to load local markup with `setHtml`.
- How to receive page messages with `setOnMessage`.
- How the navigation and title callbacks are reserved for a future native delegate.
- How corner radius follows the theme.

## The words you need

- **WebView** — a widget that displays web content. Fluffy uses a native web view,
  so the page is rendered by the operating system's web engine inside the
  widget's rectangle.
- **Navigation** — moving the web view to a URL. Loading a page, clicking a link,
  or redirecting to another site are all navigations.
- **Page message** — a JSON string that the web page sends to your C++ program.
  The page decides when to send it; your code supplies a callback to receive it.
- **Corner radius** — how rounded the corners of the view are. The default value
  follows the current theme, so a light or dark window keeps a consistent shape.

## The full program

```cpp
#include <cstdio>
#include <string>

#include <fluffy/webview.hpp>
#include <fluffy/window.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("WebView", 680, 380);
    if (!win) return 1;

    auto& wv = win.add<fluffy::WebView>();
    wv.setBounds(fluffy::Rect{40, 40, 600, 300});
    wv.navigate("https://example.com");
    wv.setOnMessage([](const std::string& json, fluffy::WebView&) {
        std::printf("msg %s\n", json.c_str());
    });

    win.run();
}
```

## Build and run

Compile this program the same way you compiled the first tutorial, then run the
resulting executable. This example loads a page from the network, so it needs
internet access. On Linux, install `libwebkit2gtk-4.1-dev` and run the program on
X11; Windows needs the WebView2 Runtime. If you need the build commands, revisit
[Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Place the view

```cpp
auto& wv = win.add<fluffy::WebView>();
wv.setBounds(fluffy::Rect{40, 40, 600, 300});
```

`win.add<fluffy::WebView>()` creates the view inside the window and returns a
reference, which we keep in `wv`. `setBounds` gives it a rectangle with a
40-pixel margin around a 600 by 300 content area.

### 2. Load a page

```cpp
wv.navigate("https://example.com");
```

`navigate` tells the web view which URL to show. This is the line that turns the
empty view into actual web content.

### 3. Listen for messages from the page

```cpp
wv.setOnMessage([](const std::string& json, fluffy::WebView&) {
    std::printf("msg %s\n", json.c_str());
});
```

`setOnMessage` installs a callback for messages sent by the web content. The first
argument is the JSON string. The second gives you the `WebView` that sent it,
which this example ignores. Messages only arrive if the page is written to send
them; a normal website may never call your callback.

The page uses the platform's own bridge:

```js
// macOS and Linux
window.webkit.messageHandlers.fluffy.postMessage(JSON.stringify({type: 'hello'}));

// Windows
window.chrome.webview.postMessage(JSON.stringify({type: 'hello'}));
```

### 4. Keep rounded corners in sync with the theme

```cpp
wv.setCornerRadius(12);
```

The default radius is `-1`, which means "follow the current theme". Call
`setCornerRadius(12)` when you want a fixed radius instead. Because the theme is
stored on the window, switching between light and dark keeps the rest of the
interface consistent, and the default corners follow along.

### 5. Load your own HTML

```cpp
wv.setHtml("<h1>Hello</h1>");
```

Use `setHtml` when you want to display markup that you provide instead of a page
from the network. It replaces whatever the view was showing, and it accepts the
same HTML, CSS, and JavaScript a browser would.

### 6. Filter navigation

```cpp
wv.setOnNavigation([](const std::string& url) {
    return url.find("example.com") != std::string::npos;
});
```

`setOnNavigation` is the API for inspecting a URL before the view follows it.
Return `true` to allow the navigation and `false` to block it. The callback is
stored, but the current native backends do not deliver navigation events yet, so
the example above is a shape for future use rather than an active filter.

`setOnTitle` is the related callback for page title changes. It receives the new
title as a string. Like navigation, it is reserved for a future native delegate
and is not delivered yet.

## Exercises

1. Load your own HTML with `wv.setHtml("<h1>Hello</h1>")`.
2. Allow navigation only to URLs containing `example.com`.
3. Use `setOnTitle` to print the page title when it changes.

## Next

Continue with [Multiwindow →](10-multiwindow.md) to run more than one window at
the same time.
