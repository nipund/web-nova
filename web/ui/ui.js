// Browser-side UI for the port. Loaded by shell.html as a module; the C++ side
// (sdl_dialog_web.cpp) talks to it through window.novaUI.

import { html, render } from "./deps.js";
import { importFromDisk, listPilots } from "./fs.js";
import { PilotDialog } from "./pilot-dialog.js";

const stylesheet = document.createElement("link");
stylesheet.rel = "stylesheet";
stylesheet.href = new URL("./ui.css", import.meta.url).href;
document.head.appendChild(stylesheet);

// Mounts view(done) in a fresh container on <body>; resolves with whatever is
// passed to done() (a promise is adopted), after unmounting.
function openModal(view) {
  return new Promise((resolve) => {
    const host = document.createElement("div");
    document.body.appendChild(host);
    const done = (value) => {
      render(null, host);
      host.remove();
      const canvas = document.getElementById("canvas");
      if (canvas) canvas.focus();
      resolve(value);
    };
    render(view(done), host);
  });
}

// SDL_ShowOpenFileDialog replacement. `accept` is an HTML accept string
// (".plt,.rez"). Resolves to a path in the virtual filesystem, or null.
function openFileDialog(accept) {
  if (!accept.toLowerCase().includes(".plt")) return importFromDisk(accept);
  return openModal((done) => html`
    <${PilotDialog}
      pilots=${listPilots()}
      onPick=${done}
      onImport=${() => done(importFromDisk(accept))}
      onCancel=${() => done(null)} />`);
}

window.novaUI = { openFileDialog };
