#!/usr/bin/env python3
"""Serve a build directory locally: scripts/serve.py [dir] [port]."""
import functools
import http.server
import sys

directory = sys.argv[1] if len(sys.argv) > 1 else "build/wasm-release/site"
port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".wasm": "application/wasm", ".js": "text/javascript"}

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


print(f"serving {directory} on http://localhost:{port}")
http.server.ThreadingHTTPServer(
    ("127.0.0.1", port), functools.partial(Handler, directory=directory)
).serve_forever()
