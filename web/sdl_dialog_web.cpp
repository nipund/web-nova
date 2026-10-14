// SDL3's Emscripten backend ships no file-dialog implementation, but the game
// links SDL_ShowOpenFileDialog / SDL_ShowFileDialogWithProperties (pilot import
// and the "locate install" dialog). These definitions open the browser's own
// file picker, copy the chosen file into the virtual filesystem under
// /tmp/webnova-import/, and report that path through the SDL callback.
//
// The pilot dialog (".plt" filter) first lists the pilots already saved in the
// virtual filesystem (the IndexedDB-backed Pilots folder) and only falls back
// to the browser's file picker for importing a pilot from disk.
//
// Weak, so a future SDL that implements the dialogs takes precedence.

#include <SDL3/SDL.h>
#include <emscripten.h>

#include <cstdlib>
#include <memory>
#include <string>

namespace {

struct Request {
  SDL_DialogFileCallback callback;
  void *userdata;
};

} // namespace

extern "C" {

// Called from the JS side once the user has picked a file (path != nullptr) or
// dismissed the picker (path == nullptr).
EMSCRIPTEN_KEEPALIVE void webnova_dialog_done(void *request, const char *path) {
  std::unique_ptr<Request> req{static_cast<Request *>(request)};
  if (path != nullptr) {
    const char *list[] = {path, nullptr};
    req->callback(req->userdata, list, -1);
  } else {
    const char *list[] = {nullptr};
    req->callback(req->userdata, list, -1);
  }
}

EM_JS(void, webnova_open_picker, (void *request, const char *accept), {
  const acceptStr = UTF8ToString(accept);
  let settled = false;
  const finish = (path) => {
    if (settled) return;
    settled = true;
    ccall("webnova_dialog_done", null, ["number", "string"], [request, path]);
  };
  const pickFromDisk = () => {
    const input = document.createElement("input");
    input.type = "file";
    if (acceptStr) input.accept = acceptStr;
    input.addEventListener("change", async () => {
      const file = input.files && input.files[0];
      if (!file) return finish(null);
      try {
        const dir = "/tmp/webnova-import";
        FS.mkdirTree(dir);
        const dest = dir + "/" + file.name.replace(/[^A-Za-z0-9._ -]/g, "_");
        FS.writeFile(dest, new Uint8Array(await file.arrayBuffer()));
        finish(dest);
      } catch (e) {
        console.error("file import failed", e);
        finish(null);
      }
    });
    input.addEventListener("cancel", () => finish(null));
    input.click();
  };

  // Saved pilots live in <SDL pref path>/Pilots, which is mounted from IndexedDB.
  const pilotsDir = "/libsdl/Ambrosia Software/EV Nova/Pilots";
  let pilots = null;
  if (acceptStr.toLowerCase().includes(".plt")) {
    try {
      pilots = FS.readdir(pilotsDir).filter((n) => n.toLowerCase().endsWith(".plt")).sort(
          (a, b) => FS.stat(pilotsDir + "/" + b).mtime - FS.stat(pilotsDir + "/" + a).mtime);
    } catch (e) {
      pilots = [];
    }
  }
  if (pilots === null) return pickFromDisk();

  const css = (el, style) => { Object.assign(el.style, style); return el; };
  const overlay = css(document.createElement("div"), {
    position: "fixed", inset: "0", zIndex: "1000", display: "grid", placeItems: "center",
    background: "rgba(0,0,0,.75)", font: "13px Geneva, Verdana, sans-serif", color: "#e6e6e6"});
  const panel = css(document.createElement("div"), {
    width: "min(420px, 92vw)", maxHeight: "80vh", display: "flex", flexDirection: "column",
    gap: "10px", padding: "12px", border: "2px solid #000",
    background: "linear-gradient(180deg, #8a8a8a, #5c5c5c 8%, #5c5c5c 92%, #3a3a3a)",
    boxShadow: "inset 1px 1px 0 rgba(255,255,255,.35), 0 12px 40px rgba(0,0,0,.8)"});
  const button = (label, onclick, extra) => {
    const b = css(document.createElement("button"), Object.assign({
      padding: "3px 18px 4px", border: "1px solid #1a0000", borderRadius: "999px",
      background: "linear-gradient(180deg, #e24a3a, #a61814 45%, #4a0504 55%, #7a0e0a)",
      boxShadow: "inset 0 1px 0 rgba(255,255,255,.45), inset 0 -1px 0 rgba(0,0,0,.5)",
      color: "#fff", font: "bold 13px/1.5 Geneva, Verdana, sans-serif",
      textShadow: "0 1px 1px rgba(0,0,0,.8)", cursor: "pointer"}, extra || {}));
    b.textContent = label;
    b.addEventListener("click", onclick);
    return b;
  };
  const close = (path) => {
    overlay.remove();
    finish(path);
    const canvas = document.getElementById("canvas");
    if (canvas) canvas.focus();
  };
  panel.appendChild(css(document.createElement("div"), {fontSize: "17px", fontWeight: "bold", textAlign: "center", padding: "5px 8px", color: "#f2f2f2",
          background: "linear-gradient(180deg, #2c2c2c, #0d0d0d)", border: "1px solid #000"}))
      .textContent = "Open Pilot";
  const list = css(document.createElement("div"), {
    display: "flex", flexDirection: "column", gap: "6px", overflowY: "auto", padding: "10px",
    background: "#050505", border: "1px solid #000"});
  if (pilots.length === 0) {
    list.appendChild(css(document.createElement("div"), {color: "#a3a3a3"}))
        .textContent = "No saved pilots in this browser yet.";
  }
  for (const name of pilots) {
    list.appendChild(button(name.slice(0, -4), () => close(pilotsDir + "/" + name)));
  }
  panel.appendChild(list);
  const row = css(document.createElement("div"), {display: "flex", gap: "10px", flexWrap: "wrap", justifyContent: "center"});
  row.appendChild(button("Import from file…", () => { overlay.remove(); pickFromDisk(); }));
  row.appendChild(button("Cancel", () => close(null)));
  panel.appendChild(row);
  overlay.appendChild(panel);
  // Keep game key handlers from seeing keystrokes meant for the dialog.
  overlay.addEventListener("keydown", (e) => {
    e.stopPropagation();
    if (e.key === "Escape") close(null);
  }, true);
  overlay.addEventListener("keyup", (e) => e.stopPropagation(), true);
  document.body.appendChild(overlay);
  const first = list.querySelector("button");
  (first || row.querySelector("button")).focus();
});

__attribute__((weak)) void SDL_ShowFileDialogWithProperties(
    SDL_FileDialogType type, SDL_DialogFileCallback callback, void *userdata,
    SDL_PropertiesID props) {
  if (type != SDL_FILEDIALOG_OPENFILE) {
    SDL_SetError("only 'open file' dialogs are supported in the browser");
    const char *const none = nullptr;
    callback(userdata, &none, -1);
    return;
  }
  int count = 0;
  const auto *filters = static_cast<const SDL_DialogFileFilter *>(
      SDL_GetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, nullptr));
  count = static_cast<int>(
      SDL_GetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, 0));
  std::string accept;
  for (int i = 0; filters != nullptr && i < count; ++i) {
    std::string pattern = filters[i].pattern;
    if (pattern == "*") {
      accept.clear();
      break;
    }
    // SDL patterns are "ext1;ext2"; the HTML accept attribute wants ".ext1,.ext2".
    std::string ext;
    for (const char c : pattern + ";") {
      if (c == ';') {
        if (!ext.empty()) {
          accept += (accept.empty() ? "." : ",.") + ext;
        }
        ext.clear();
      } else {
        ext += c;
      }
    }
  }
  webnova_open_picker(new Request{callback, userdata}, accept.c_str());
}

__attribute__((weak)) void SDL_ShowOpenFileDialog(
    SDL_DialogFileCallback callback, void *userdata, SDL_Window *window,
    const SDL_DialogFileFilter *filters, int nfilters, const char *default_location,
    bool allow_many) {
  const SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER,
                         const_cast<SDL_DialogFileFilter *>(filters));
  SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, nfilters);
  SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, window);
  SDL_SetStringProperty(props, SDL_PROP_FILE_DIALOG_LOCATION_STRING, default_location);
  SDL_SetBooleanProperty(props, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, allow_many);
  SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, callback, userdata, props);
  SDL_DestroyProperties(props);
}

} // extern "C"
