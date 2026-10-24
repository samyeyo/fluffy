#!/usr/bin/env python3
# www/gendocs.py
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# Generate the Fluffy website documentation and tutorial pages from the
# Markdown sources in doc/ and doc/tutorial/.  Output is static HTML sharing
# the theme in www/assets.  Run from anywhere:
#
#     python3 www/gendocs.py            # sync checked-in screenshots
#     python3 www/gendocs.py --images   # rebuild them first
#
# Widget screenshots live in www/assets/widgets/ so the website tree is
# self-contained; doc/*.md references them through ../www/assets/widgets/.
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

import argparse
import html
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

try:
    import markdown
except ImportError:
    sys.exit("error: the 'markdown' Python package is required (pip install markdown)")

ROOT = Path(__file__).resolve().parent.parent
DOC_DIR = ROOT / "doc"
TUT_DIR = DOC_DIR / "tutorial"
OUT_DOCS = ROOT / "www" / "documentation"
OUT_TUTS = ROOT / "www" / "tutorials"

# Public site URL, used for canonical links, social metadata and the sitemap.
# Override with FLUFFY_SITE_URL when publishing to another domain.
SITE_URL = os.environ.get("FLUFFY_SITE_URL", "https://samyeyo.github.io/fluffy/")
if not SITE_URL.endswith("/"):
    SITE_URL += "/"
SITE_NAME = "Fluffy"
SITE_AUTHOR = "Samir Tine"
SITE_IMAGE = "fluffy-light.png"
SITE_IMAGE_WIDTH = 819
SITE_IMAGE_HEIGHT = 390

sys.path.insert(0, str(Path(__file__).resolve().parent))
import snippets  # noqa: E402

SKIP = {"book.md"}

# ––––––––[ SNIPPETS ] ordered runnable programs per Markdown file stem
SNIPPETS = snippets.by_file()

# ––––––––[ GROUPS ] documentation sidebar order and grouping
GROUPS = [
    ("Getting started", ["readme", "install", "geom", "widget", "custom", "window", "application"]),
    ("Core widgets", ["label", "button", "textinput", "image"]),
    ("Layout", ["container", "layouts"]),
    ("Toggles & pickers", ["checkbox", "radiobutton", "switch", "slider", "color_picker"]),
    ("Data widgets", ["combobox", "listview", "tabview", "table", "progress_ring"]),
    ("Navigation & overlays", ["menu", "nav", "tooltip", "dialog", "webview"]),
    ("Styling", ["theme", "inline", "icons"]),
    ("Declarative UI", ["yaml"]),
]

DOC_LABELS = {
    "readme": "Welcome to Fluffy",
    "install": "Installing Fluffy",
    "geom": "Geometry & colors",
    "widget": "Widget basics",
    "custom": "Custom widgets",
    "window": "Window",
    "application": "Application",
    "label": "Label",
    "button": "Button",
    "textinput": "TextInput",
    "image": "Image",
    "container": "Container",
    "layouts": "Layouts",
    "checkbox": "Checkbox",
    "radiobutton": "RadioButton",
    "switch": "Switch",
    "slider": "Slider",
    "color_picker": "ColorPicker",
    "combobox": "ComboBox",
    "listview": "ListView",
    "tabview": "TabView",
    "table": "Table",
    "progress_ring": "ProgressRing",
    "menu": "Menu",
    "nav": "Nav",
    "tooltip": "Tooltip",
    "dialog": "Dialogs",
    "webview": "WebView",
    "theme": "Theme",
    "inline": "Inline",
    "icons": "Icons",
    "yaml": "YAML UI",
}

TUT_ORDER = [
    "readme", "01-hello-fluffy", "02-counter", "03-input-form", "04-layouts",
    "05-icons-and-themes", "06-yaml-ui", "07-dialogs", "08-data-widgets",
    "09-webview", "10-multiwindow", "11-custom-widget",
]

TUT_LABELS = {
    "readme": "Tutorial overview",
    "01-hello-fluffy": "1 · Hello Fluffy",
    "02-counter": "2 · Counter",
    "03-input-form": "3 · Input Form",
    "04-layouts": "4 · Layouts",
    "05-icons-and-themes": "5 · Icons & Themes",
    "06-yaml-ui": "6 · YAML UI",
    "07-dialogs": "7 · Dialogs",
    "08-data-widgets": "8 · Data Widgets",
    "09-webview": "9 · WebView",
    "10-multiwindow": "10 · Multiwindow",
    "11-custom-widget": "11 · Custom Widget",
}


# ––––––––[ slugify ] turn a heading into a url-safe anchor id
def slugify(text):
    text = re.sub(r"<[^>]+>", "", text)
    text = html.unescape(text).lower()
    text = re.sub(r"[^\w\s-]", "", text)
    text = re.sub(r"[\s_]+", "-", text).strip("-")
    return text or "section"


# ––––––––[ strip_md ] plain-text version of a markdown snippet
def strip_md(text):
    text = re.sub(r"`([^`]*)`", r"\1", text)
    text = re.sub(r"\[([^\]]+)\]\([^)]*\)", r"\1", text)
    text = re.sub(r"[*_#>]", "", text)
    text = re.sub(r"<[^>]+>", "", text)
    return re.sub(r"\s+", " ", text).strip()


# ––––––––[ rewrite_links ] point .md links at the generated .html pages
def rewrite_links(body, section):
    doc_slugs = set(DOC_LABELS)

    def page_name(slug):
        return "index" if slug == "readme" else slug

    def repl(match):
        href = match.group(1)
        if href.startswith(("http://", "https://", "mailto:", "#")):
            return match.group(0)
        anchor = ""
        target = href
        if "#" in href:
            target, anchor = href.split("#", 1)
            anchor = "#" + anchor
        if not target.endswith(".md"):
            return match.group(0)
        stem = target[: -len(".md")]
        prefix = ""
        name = stem
        if stem.startswith("../documentation/"):
            prefix, name = "../documentation/", stem.split("/")[-1]
        elif stem.startswith("../"):
            name = stem.split("/")[-1]
            if name in doc_slugs:
                prefix = "../documentation/"
            else:
                prefix = "../"
        elif section == "docs":
            if stem.startswith("tutorial/"):
                prefix, name = "../tutorials/", stem.split("/")[-1]
            else:
                prefix = "" if stem in doc_slugs else "../documentation/"
        elif section == "tutorials":
            if stem in doc_slugs:
                prefix = "../documentation/"
            else:
                prefix = ""
        return 'href="{}{}{}"'.format(prefix, page_name(name) + ".html", anchor)

    return re.sub(r'href="([^"]+)"', repl, body)


# ––––––––[ convert ] markdown -> html with heading anchors and a toc
def convert(text, section, snippet_ids=None):
    body = markdown.markdown(
        text,
        extensions=["fenced_code", "tables", "sane_lists"],
        output_format="html5",
    )

    # ––––––––[ data-try ] every complete C++ program gets a Try it button.
    # The ids come from www/snippets.py in the same order the fences appear,
    # so each button opens the WebAssembly build of that exact program.
    pending = list(snippet_ids or [])

    def tag_program(match):
        attrs, inner = match.group(1), match.group(2)
        if "data-try=" in attrs or "int main(" not in inner or not pending:
            return match.group(0)
        snippet = pending.pop(0)
        return '<pre data-try="{}" data-w="{}" data-h="{}"{}>{}</pre>'.format(
            snippet["id"], snippet["width"], snippet["height"], attrs, inner)

    body = re.sub(
        r'<pre([^>]*)>(<code class="language-cpp">.*?</code>)</pre>',
        tag_program,
        body,
        flags=re.DOTALL,
    )
    body = rewrite_links(body, section)
    toc = []
    seen = {}

    def repl(match):
        level, inner = match.group(1), match.group(2)
        slug = slugify(inner)
        seen[slug] = seen.get(slug, 0) + 1
        if seen[slug] > 1:
            slug = "{}-{}".format(slug, seen[slug])
        toc.append((int(level), slug, strip_md(inner)))
        return '<h{0} id="{1}">{2}</h{0}>'.format(level, slug, inner)

    body = re.sub(r"<h([23])>(.*?)</h\1>", repl, body)
    body = decorate_images(body)
    return body, toc


# ––––––––[ title_of ] first level-1 heading of a markdown file
def title_of(text, fallback):
    match = re.search(r"^#\s+(.+)$", text, re.MULTILINE)
    return strip_md(match.group(1)) if match else fallback


# ––––––––[ description_of ] first paragraph, for meta/description text
def description_of(text):
    lines = text.splitlines()
    i = 0
    while i < len(lines) and not lines[i].startswith("# "):
        i += 1
    i += 1
    paragraph = []
    for line in lines[i:]:
        s = line.strip()
        if not s:
            if paragraph:
                break
            continue
        if not paragraph and s[0] in "#->|`*":
            continue
        if s.startswith("```"):
            break
        paragraph.append(s)
    description = strip_md(" ".join(paragraph))
    if not description:
        return "Fluffy — the soft answer to stiff, native UI frameworks."
    if len(description) > 180:
        match = re.match(r"(.+?[.!?])(?:\s|$)", description)
        if match and len(match.group(1)) <= 200:
            return match.group(1)
        description = description[:177].rsplit(" ", 1)[0].rstrip(" ,;:-") + "…"
    return description


# ––––––––[ page_rel ] canonical path of a generated page, relative to the site
def page_rel(section, slug):
    folder = "documentation" if section == "docs" else "tutorials"
    name = "" if slug == "readme" else slug + ".html"
    return folder + "/" + name


# ––––––––[ absolute_url ] join a site-relative path to the public URL
def absolute_url(path=""):
    return SITE_URL + path


# ––––––––[ seo_head ] canonical, social cards, theme colors and JSON-LD
def seo_head(root, title, description, canonical, kind="article"):
    image = absolute_url(SITE_IMAGE)
    if kind == "home":
        structured = {
            "@context": "https://schema.org",
            "@type": "SoftwareApplication",
            "name": SITE_NAME,
            "applicationCategory": "DeveloperApplication",
            "operatingSystem": "macOS, Windows, Linux",
            "description": description,
            "url": canonical,
            "image": image,
            "license": "https://opensource.org/licenses/MIT",
            "offers": {"@type": "Offer", "price": "0", "priceCurrency": "USD"},
            "author": {"@type": "Person", "name": SITE_AUTHOR},
        }
        og_type = "website"
    elif kind == "collection":
        structured = {
            "@context": "https://schema.org",
            "@type": "CollectionPage",
            "name": title,
            "description": description,
            "url": canonical,
            "inLanguage": "en",
            "author": {"@type": "Person", "name": SITE_AUTHOR},
            "isPartOf": {"@type": "WebSite", "name": SITE_NAME, "url": SITE_URL},
            "image": image,
        }
        og_type = "website"
    else:
        structured = {
            "@context": "https://schema.org",
            "@type": "TechArticle",
            "headline": title,
            "description": description,
            "url": canonical,
            "mainEntityOfPage": {"@type": "WebPage", "@id": canonical},
            "inLanguage": "en",
            "author": {"@type": "Person", "name": SITE_AUTHOR},
            "publisher": {"@type": "Organization", "name": SITE_NAME, "url": SITE_URL},
            "isPartOf": {"@type": "WebSite", "name": SITE_NAME, "url": SITE_URL},
            "image": image,
        }
        og_type = "article"
    json_ld = json.dumps(structured, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    return f"""<link rel="canonical" href="{html.escape(canonical)}">
<meta name="robots" content="index, follow, max-image-preview:large">
<meta name="author" content="{SITE_AUTHOR}">
<meta name="theme-color" content="#FFFFFF" media="(prefers-color-scheme: light)">
<meta name="theme-color" content="#000000" media="(prefers-color-scheme: dark)">
<meta name="color-scheme" content="light dark">
<meta property="og:locale" content="en_US">
<meta property="og:type" content="{og_type}">
<meta property="og:site_name" content="{SITE_NAME}">
<meta property="og:title" content="{html.escape(title)}">
<meta property="og:description" content="{html.escape(description)}">
<meta property="og:url" content="{html.escape(canonical)}">
<meta property="og:image" content="{html.escape(image)}">
<meta property="og:image:width" content="{SITE_IMAGE_WIDTH}">
<meta property="og:image:height" content="{SITE_IMAGE_HEIGHT}">
<meta property="og:image:alt" content="Fluffy — a modern C++ desktop UI toolkit">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="{html.escape(title)}">
<meta name="twitter:description" content="{html.escape(description)}">
<meta name="twitter:image" content="{html.escape(image)}">
<link rel="icon" href="{root}favicon.svg" type="image/svg+xml">
<script type="application/ld+json">{json_ld}</script>"""


# ––––––––[ png_size ] intrinsic dimensions for a generated documentation image
def png_size(src):
    if not src.startswith("../assets/"):
        return None
    path = ROOT / "www" / src[3:]
    try:
        data = path.read_bytes()[:24]
    except OSError:
        return None
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    return struct.unpack(">II", data[16:24])


# ––––––––[ decorate_images ] lazy-load, decode off-thread, reserve aspect ratio
def decorate_images(body):
    def repl(match):
        tag = match.group(0)
        if "loading=" not in tag:
            tag = tag.replace("<img ", '<img loading="lazy" decoding="async" ', 1)
        size = re.search(r'src="([^"]+)"', tag)
        if size and "width=" not in tag:
            dims = png_size(size.group(1))
            if dims:
                tag = tag.replace(
                    "<img ",
                    '<img width="{}" height="{}" '.format(dims[0], dims[1]),
                    1,
                )
        return tag

    return re.sub(r"<img\b[^>]*>", repl, body)


# ––––––––[ href ] resolve a page file name relative to the site root
def rel_href(section, slug):
    if section == "docs":
        name = "index.html" if slug == "readme" else slug + ".html"
        return "documentation/" + name
    name = "index.html" if slug == "readme" else slug + ".html"
    return "tutorials/" + name


def rel_href_page(section, slug, root):
    return root + rel_href(section, slug)


# ––––––––[ header_html ] shared site header
def header_html(root, active):
    def cls(name):
        return ' class="active"' if active == name else ""

    return f"""<header class="site-header">
  <div class="wrap">
    <a class="brand" href="{root}index.html">
      <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
        <defs><linearGradient id="logoGrad" x1="0" y1="12" x2="24" y2="12" gradientUnits="userSpaceOnUse">
          <stop stop-color="#2FE1E8"/><stop offset="1" stop-color="#9B5CFF"/>
        </linearGradient></defs>
        <path fill="url(#logoGrad)" d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"/>
      </svg>
      Fluffy
    </a>
    <button class="icon-btn nav-toggle" type="button" aria-label="Toggle menu">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M4 7h16M4 12h16M4 17h16"/></svg>
    </button>
    <nav class="nav-links">
      <a href="{root}index.html">Home</a>
      <a href="{root}documentation/index.html"{cls('docs')}>Docs</a>
      <a href="{root}tutorials/index.html"{cls('tutorials')}>Tutorials</a>
      <button class="icon-btn theme-toggle" type="button" aria-label="Toggle dark mode">
        <svg class="sun" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></svg>
        <svg class="moon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8Z"/></svg>
      </button>
    </nav>
  </div>
</header>"""


# ––––––––[ footer_html ] shared site footer
def footer_html(root):
    return f"""<footer class="site-footer">
  <div class="wrap">
    <div class="cols">
      <div class="about">
        <a class="brand" href="{root}index.html" style="font-size:20px">
          <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" aria-hidden="true" style="width:30px;height:30px">
            <defs><linearGradient id="footGrad" x1="0" y1="12" x2="24" y2="12" gradientUnits="userSpaceOnUse">
              <stop stop-color="#2FE1E8"/><stop offset="1" stop-color="#9B5CFF"/>
            </linearGradient></defs>
            <path fill="url(#footGrad)" d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"/>
          </svg>
          Fluffy
        </a>
        <div class="about-copy">
          <p>The soft answer to stiff, native UI frameworks. MIT licensed — because software should be free, fluffy and fun.</p>
          <span class="copyright">© 2026 Tine Samir</span>
        </div>
      </div>
    </div>
  </div>
</footer>"""


# ––––––––[ sidebar_html ] documentation / tutorial navigation tree
def sidebar_html(root, section, current):
    parts = ['<aside class="docs-sidebar">']
    if section == "docs":
        for group, slugs in GROUPS:
            parts.append('<div class="group"><span>{}</span>'.format(html.escape(group)))
            for slug in slugs:
                href = rel_href_page("docs", slug, root)
                active = ' class="active"' if slug == current else ""
                parts.append('<a href="{}"{}>{}</a>'.format(
                    href, active, html.escape(DOC_LABELS.get(slug, slug))))
            parts.append("</div>")
    else:
        parts.append('<div class="group"><span>Tutorials</span>')
        for slug in TUT_ORDER:
            href = rel_href_page("tutorials", slug, root)
            active = ' class="active"' if slug == current else ""
            parts.append('<a href="{}"{}>{}</a>'.format(
                href, active, html.escape(TUT_LABELS.get(slug, slug))))
        parts.append("</div>")
        parts.append('<div class="group"><span>Reference</span>')
        for label, slug in [("Window", "window"), ("Widgets", "widget"), ("Theme", "theme"), ("YAML UI", "yaml")]:
            parts.append('<a href="{}documentation/{}.html">{}</a>'.format(root, slug, html.escape(label)))
        parts.append("</div>")
    parts.append("</aside>")
    return "\n  ".join(parts)


# ––––––––[ toc_html ] the "on this page" rail
def toc_html(toc):
    if not toc:
        return ""
    items = ["<span>On this page</span>"]
    for level, slug, title in toc:
        cls = "lvl-3" if level == 3 else "lvl-2"
        items.append('<a class="{}" href="#{}">{}</a>'.format(cls, slug, html.escape(title)))
    return '<aside class="docs-toc">\n    ' + "\n    ".join(items) + "\n  </aside>"


# ––––––––[ pager_html ] previous / next navigation
def pager_html(root, section, current):
    if section == "docs":
        order = [s for _, slugs in GROUPS for s in slugs]
        labels = DOC_LABELS
    else:
        order = TUT_ORDER
        labels = TUT_LABELS
    if current not in order:
        return ""
    idx = order.index(current)
    parts = ['<nav class="pager">']
    if idx > 0:
        slug = order[idx - 1]
        parts.append('<a class="prev" href="{}"><span class="dir">← Previous</span>'
                     '<span class="ttl">{}</span></a>'.format(
                         rel_href_page(section, slug, root), html.escape(labels.get(slug, slug))))
    else:
        parts.append("<span></span>")
    if idx < len(order) - 1:
        slug = order[idx + 1]
        parts.append('<a class="next" href="{}"><span class="dir">Next →</span>'
                     '<span class="ttl">{}</span></a>'.format(
                         rel_href_page(section, slug, root), html.escape(labels.get(slug, slug))))
    parts.append("</nav>")
    return "\n    ".join(parts)


# ––––––––[ page_html ] full standalone page
def page_html(root, section, slug, title, description, content, toc, crumb):
    active = "docs" if section == "docs" else "tutorials"
    full_title = title if "Fluffy" in title else title + " · Fluffy"
    canonical = absolute_url(page_rel(section, slug))
    kind = "collection" if slug == "readme" else "article"
    return f"""<!DOCTYPE html>
<html lang="en" data-root="{root}">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{html.escape(full_title)}</title>
<meta name="description" content="{html.escape(description)}">
{seo_head(root, full_title, description, canonical, kind)}
<script>
  (function () {{
    try {{
      var t = localStorage.getItem('fluffy-theme');
      if (t === 'light' || t === 'dark') document.documentElement.setAttribute('data-theme', t);
    }} catch (e) {{ /* ignore */ }}
  }})();
</script>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="preload" as="style" href="https://fonts.googleapis.com/css2?family=Baloo+2:wght@500;600;700;800&amp;family=Nunito:wght@400;500;600;700;800&amp;family=JetBrains+Mono:wght@400;500;600&amp;display=swap">
<link href="https://fonts.googleapis.com/css2?family=Baloo+2:wght@500;600;700;800&amp;family=Nunito:wght@400;500;600;700;800&amp;family=JetBrains+Mono:wght@400;500;600&amp;display=swap" rel="stylesheet" media="print" onload="this.media='all'">
<noscript><link href="https://fonts.googleapis.com/css2?family=Baloo+2:wght@500;600;700;800&amp;family=Nunito:wght@400;500;600;700;800&amp;family=JetBrains+Mono:wght@400;500;600&amp;display=swap" rel="stylesheet"></noscript>
<link rel="stylesheet" href="{root}assets/site.css?v=21">
<script src="{root}assets/highlight.min.js" defer></script>
<script src="{root}assets/site.js?v=11" defer></script>
</head>
<body>

{header_html(root, active)}

<div class="docs-shell">
  {sidebar_html(root, section, slug)}

  <main class="docs-main">
    <nav class="breadcrumb">
      <a href="{root}index.html">Fluffy</a>
      <span class="sep">/</span>
      <a href="{root}{'documentation' if section == 'docs' else 'tutorials'}/index.html">{crumb}</a>
      <span class="sep">/</span>
      <span>{html.escape(title)}</span>
    </nav>
    <article class="prose">
      <h1>{html.escape(title)}</h1>
{content}
    </article>
    {pager_html(root, section, slug)}
  </main>

  {toc_html(toc)}
</div>

{footer_html(root)}

</body>
</html>
"""


# ––––––––[ rewrite_asset_paths ] map repo-relative doc images to website paths
def rewrite_asset_paths(text):
    return text.replace("../www/assets/widgets/", "../assets/widgets/")


# ––––––––[ render_docs ] generate every documentation page
def render_docs():
    OUT_DOCS.mkdir(parents=True, exist_ok=True)
    for path in sorted(DOC_DIR.glob("*.md")):
        if path.name in SKIP:
            continue
        slug = path.stem
        text = rewrite_asset_paths(path.read_text(encoding="utf-8"))
        title = DOC_LABELS.get(slug) or title_of(text, slug)
        body, toc = convert(text, "docs", SNIPPETS.get(slug))
        body = re.sub(r"^\s*<h1>.*?</h1>\s*", "", body, count=1)
        out = OUT_DOCS / ("index.html" if slug == "readme" else slug + ".html")
        out.write_text(
            page_html("../", "docs", slug, title, description_of(text), body, toc, "Documentation"),
            encoding="utf-8",
        )
        print("  docs/{} -> {}".format(path.name, out.relative_to(ROOT)))


# ––––––––[ render_tutorials ] generate every tutorial page
def render_tutorials():
    OUT_TUTS.mkdir(parents=True, exist_ok=True)
    for path in sorted(TUT_DIR.glob("*.md")):
        if path.name in SKIP:
            continue
        slug = path.stem
        text = rewrite_asset_paths(path.read_text(encoding="utf-8"))
        title = title_of(text, TUT_LABELS.get(slug, slug))
        body, toc = convert(text, "tutorials", SNIPPETS.get("tutorial/" + slug))
        body = re.sub(r"^\s*<h1>.*?</h1>\s*", "", body, count=1)
        out = OUT_TUTS / ("index.html" if slug == "readme" else slug + ".html")
        out.write_text(
            page_html("../", "tutorials", slug, title, description_of(text), body, toc, "Tutorials"),
            encoding="utf-8",
        )
        print("  tutorial/{} -> {}".format(path.name, out.relative_to(ROOT)))


# ––––––––[ write_seo_files ] robots + sitemap for the published site
def write_seo_files():
    urls = [(SITE_URL, "1.0")]
    urls.append((absolute_url("documentation/"), "0.8"))
    for _, slugs in GROUPS:
        for slug in slugs:
            if slug != "readme":
                urls.append((absolute_url(page_rel("docs", slug)), "0.7"))
    urls.append((absolute_url("tutorials/"), "0.8"))
    for slug in TUT_ORDER:
        if slug != "readme":
            urls.append((absolute_url(page_rel("tutorials", slug)), "0.7"))

    sitemap = ['<?xml version="1.0" encoding="UTF-8"?>',
               '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">']
    for loc, priority in urls:
        sitemap.append("  <url><loc>{}</loc><priority>{}</priority></url>".format(
            html.escape(loc), priority))
    sitemap.append("</urlset>")
    (ROOT / "www" / "sitemap.xml").write_text("\n".join(sitemap) + "\n", encoding="utf-8")

    robots = "\n".join([
        "User-agent: *",
        "Allow: /",
        "Disallow: /demo/",
        "",
        "Sitemap: {}sitemap.xml".format(SITE_URL),
        "",
    ])
    (ROOT / "www" / "robots.txt").write_text(robots, encoding="utf-8")


# ––––––––[ build_widget_shots ] compile and run the documentation screenshot tool
def build_widget_shots():
    if sys.platform == "darwin":
        platform_name = "macOS"
    elif sys.platform.startswith("linux"):
        platform_name = "Linux"
    elif os.name == "nt":
        platform_name = "Windows"
    else:
        sys.exit("error: unsupported platform for --images: {}".format(sys.platform))

    source = ROOT / "www" / "widgetshots"
    build = ROOT / "build"
    work = build / "widgetshots"
    if not source.is_dir():
        sys.exit("error: missing {}".format(source.relative_to(ROOT)))
    if not build.is_dir():
        sys.exit("error: build/ is missing — run ./build.sh first")

    print("Building widgetshots for {}...".format(platform_name))
    subprocess.run(
        [
            "cmake", "-S", str(source), "-B", str(work),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DFLUFFY_ROOT={}".format(ROOT),
            "-DFLUFFY_BUILD_DIR={}".format(build),
        ],
        cwd=ROOT, check=True,
    )
    subprocess.run(
        [
            "cmake", "--build", str(work),
            "--target", "widgetshots", "--config", "Release", "--parallel",
        ],
        cwd=ROOT, check=True,
    )

    names = ["widgetshots", "widgetshots.exe"]
    configs = ["Release", "RelWithDebInfo", "Debug", "MinSizeRel"]
    candidates = [work / name for name in names]
    candidates += [work / config / name for config in configs for name in names]
    binary = next((path for path in candidates if path.is_file()), None)
    if binary is None:
        sys.exit("error: widgetshots binary not found under {}".format(work))

    subprocess.run([str(binary)], cwd=ROOT, check=True)


# ––––––––[ main ] entry point
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--images", action="store_true",
        help="rebuild widget screenshots before generating the site",
    )
    args = parser.parse_args()

    print("Generating Fluffy documentation site…")
    if args.images:
        build_widget_shots()
    render_docs()
    render_tutorials()
    write_seo_files()
    print("Done.")


if __name__ == "__main__":
    main()
