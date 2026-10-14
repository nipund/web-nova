// Everything that touches the Emscripten virtual filesystem. Components stay
// free of FS details; they only deal with the plain objects returned here.

const fs = () => window.Module.FS;

// Saved pilots live in <SDL pref path>/Pilots, which is mounted from IndexedDB.
export const PILOTS_DIR = "/libsdl/Ambrosia Software/EV Nova/Pilots";
const IMPORT_DIR = "/tmp/webnova-import";

// Saved pilots, newest first: [{ file, name, path }].
export function listPilots() {
  const FS = fs();
  try {
    const mtime = (file) => FS.stat(`${PILOTS_DIR}/${file}`).mtime;
    return FS.readdir(PILOTS_DIR)
      .filter((file) => file.toLowerCase().endsWith(".plt"))
      .sort((a, b) => mtime(b) - mtime(a))
      .map((file) => ({ file, name: file.slice(0, -4), path: `${PILOTS_DIR}/${file}` }));
  } catch (e) {
    return [];
  }
}

// Opens the browser's file picker and copies the chosen file into the virtual
// filesystem. Resolves to its path there, or null if nothing was chosen.
// Call this synchronously from a click handler so the picker is allowed to open.
export function importFromDisk(accept) {
  return new Promise((resolve) => {
    const input = document.createElement("input");
    input.type = "file";
    if (accept) input.accept = accept;
    input.addEventListener("change", async () => {
      const file = input.files && input.files[0];
      if (!file) return resolve(null);
      try {
        const FS = fs();
        FS.mkdirTree(IMPORT_DIR);
        const dest = `${IMPORT_DIR}/${file.name.replace(/[^A-Za-z0-9._ -]/g, "_")}`;
        FS.writeFile(dest, new Uint8Array(await file.arrayBuffer()));
        resolve(dest);
      } catch (e) {
        console.error("file import failed", e);
        resolve(null);
      }
    });
    input.addEventListener("cancel", () => resolve(null));
    input.click();
  });
}
