// SDL3's Emscripten backend ships no file-dialog implementation, but the game
// links SDL_ShowOpenFileDialog / SDL_ShowFileDialogWithProperties (pilot import
// and the "locate install" dialog). These definitions hand the request to the
// page's UI (web/ui/), which shows a dialog or the browser's own file picker,
// copies the chosen file into the virtual filesystem under /tmp/webnova-import/,
// and reports that path through the SDL callback.
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

// The dialogs themselves are Preact components in web/ui/; window.novaUI is
// their entry point (see web/ui/ui.js).
EM_JS(void, webnova_open_picker, (void *request, const char *accept), {
  const finish = (path) => ccall("webnova_dialog_done", null, ["number", "string"], [request, path]);
  if (!window.novaUI) {
    console.error("novaUI is not loaded; cannot open a file dialog");
    return finish(null);
  }
  window.novaUI.openFileDialog(UTF8ToString(accept)).then(finish, (e) => {
    console.error("file dialog failed", e);
    finish(null);
  });
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
