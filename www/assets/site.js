// www/assets/site.js
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Fluffy website behaviour: theme toggle, mobile nav, code copy buttons,
// docs "on this page" scroll spy. No dependencies.
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

(function () {
    'use strict';

    // ––––––––[ applyTheme ] reflect the stored or system theme on <html>
    var root = document.documentElement;
    var stored = null;
    try { stored = localStorage.getItem('fluffy-theme'); } catch (e) { stored = null; }
    if (stored === 'light' || stored === 'dark') {
        root.setAttribute('data-theme', stored);
    }

    function current() {
        var attr = root.getAttribute('data-theme');
        if (attr) return attr;
        return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    }

    // ––––––––[ demo sizes ] default popup size when a link carries none
    var DEMO_DEFAULT = { w: 920, h: 640 };

    var pageRoot = document.documentElement.getAttribute('data-root') || '';

    // ––––––––[ openDemo ] launch a WASM demo in its own compact window
    function openDemo(url, width, height) {
        var w = width || DEMO_DEFAULT.w;
        var h = height || DEMO_DEFAULT.h;
        var demo = window.open(
            url,
            'fluffy-demo',
            'popup=yes,width=' + w + ',height=' + h + ',left=' +
            Math.max(0, Math.round((screen.availWidth - w) / 2)) + ',top=' +
            Math.max(0, Math.round((screen.availHeight - h) / 2)) +
            ',resizable=yes,scrollbars=no,toolbar=no,menubar=no,location=no,status=no'
        );
        if (demo) {
            demo.focus();
            return true;
        }
        return false;
    }

    // ––––––––[ sizeOf ] read data-w/data-h from an element, if present
    function sizeOf(el) {
        return {
            w: parseInt(el.getAttribute('data-w'), 10) || 0,
            h: parseInt(el.getAttribute('data-h'), 10) || 0
        };
    }

    // ––––––––[ bindDemoLinks ] wire the nav/hero buttons to the demo launcher
    function bindDemoLinks() {
        Array.prototype.forEach.call(document.querySelectorAll('[data-demo]'), function (link) {
            link.addEventListener('click', function (event) {
                if (event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
                event.preventDefault();
                var size = sizeOf(link);
                if (!openDemo(link.href, size.w, size.h)) window.location.href = link.href;
            });
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', bindDemoLinks);
    } else {
        bindDemoLinks();
    }

    document.addEventListener('DOMContentLoaded', function () {
        var toggle = document.querySelector('.theme-toggle');
        if (toggle) {
            toggle.addEventListener('click', function () {
                var next = current() === 'dark' ? 'light' : 'dark';
                root.setAttribute('data-theme', next);
                try { localStorage.setItem('fluffy-theme', next); } catch (e) { /* ignore */ }
            });
        }

        var navToggle = document.querySelector('.nav-toggle');
        var navLinks = document.querySelector('.nav-links');
        if (navToggle && navLinks) {
            navToggle.addEventListener('click', function () {
                navLinks.classList.toggle('open');
            });
        }

        // ––––––––[ copy buttons ] add a copy control to every docs code block
        // ––––––––[ code actions ] add Copy, plus Try it for runnable snippets
        document.querySelectorAll('.prose pre').forEach(function (pre) {
            var actions = document.createElement('div');
            actions.className = 'code-actions';

            var tryId = pre.getAttribute('data-try');
            if (tryId) {
                var tryBtn = document.createElement('button');
                tryBtn.type = 'button';
                tryBtn.className = 'copy-btn try-btn';
                tryBtn.textContent = 'Try it';
                tryBtn.addEventListener('click', function () {
                    var size = sizeOf(pre);
                    var url = pageRoot + 'demo/app.html?demo=' + encodeURIComponent(tryId) +
                        (size.w ? '&w=' + size.w : '') + (size.h ? '&h=' + size.h : '') + '&v=15';
                    if (!openDemo(url, size.w, size.h)) window.location.href = url;
                });
                actions.appendChild(tryBtn);
            }

            var btn = document.createElement('button');
            btn.type = 'button';
            btn.className = 'copy-btn';
            btn.textContent = 'Copy';
            btn.addEventListener('click', function () {
                var code = pre.querySelector('code');
                var text = code ? code.innerText : pre.innerText;
                navigator.clipboard.writeText(text).then(function () {
                    btn.textContent = 'Copied!';
                    setTimeout(function () { btn.textContent = 'Copy'; }, 1400);
                });
            });
            actions.appendChild(btn);
            pre.appendChild(actions);
        });

        // ––––––––[ scroll spy ] highlight the active entry in the docs TOC
        var tocLinks = Array.prototype.slice.call(document.querySelectorAll('.docs-toc a'));
        if (tocLinks.length && 'IntersectionObserver' in window) {
            var map = {};
            tocLinks.forEach(function (a) {
                var id = a.getAttribute('href').slice(1);
                var el = document.getElementById(id);
                if (el) map[id] = a;
            });
            var spy = new IntersectionObserver(function (entries) {
                entries.forEach(function (en) {
                    if (en.isIntersecting && map[en.target.id]) {
                        tocLinks.forEach(function (a) { a.classList.remove('active'); });
                        map[en.target.id].classList.add('active');
                    }
                });
            }, { rootMargin: '-84px 0px -75% 0px', threshold: 0 });
            Object.keys(map).forEach(function (id) {
                spy.observe(document.getElementById(id));
            });
        }

        if (window.hljs) {
            window.hljs.highlightAll();
        }
    });
})();
