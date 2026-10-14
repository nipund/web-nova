import { html, useEffect, useRef, useState } from "./deps.js";
import { Modal } from "./modal.js";
import { deletePilot, exportPilot, listPilots } from "./fs.js";

// One saved pilot: pick it, export it, or delete it (after an inline confirm).
function PilotRow({ pilot, confirming, onPick, onExport, onAskDelete, onDelete, onKeep }) {
  const root = useRef(null);
  const first = useRef(true);
  // Keep keyboard focus on a sensible control when the row swaps modes.
  useEffect(() => {
    if (first.current) { first.current = false; return; }
    const target = root.current.querySelector(confirming ? ".nova-keep" : ".nova-ask-delete");
    if (target) target.focus();
  }, [confirming]);

  if (confirming) {
    return html`
      <div class="nova-item nova-confirming" ref=${root}>
        <span class="nova-confirm-text">Delete ${pilot.name}?</span>
        <button class="nova-act nova-delete" onClick=${onDelete}>Delete</button>
        <button class="nova-act nova-keep" onClick=${onKeep}>Keep</button>
      </div>`;
  }
  return html`
    <div class="nova-item" ref=${root}>
      <button class="nova-row" onClick=${onPick}>${pilot.name}</button>
      <button class="nova-act" title="Export" aria-label=${"Export " + pilot.name} onClick=${onExport}>↓</button>
      <button class="nova-act nova-ask-delete" title="Delete" aria-label=${"Delete " + pilot.name}
              onClick=${onAskDelete}>✕</button>
    </div>`;
}

export function PilotDialog({ onPick, onImport, onCancel }) {
  const [pilots, setPilots] = useState(listPilots);
  const [confirming, setConfirming] = useState(null);   // file name awaiting delete confirmation
  const [error, setError] = useState("");
  const panel = useRef(null);

  const remove = async (pilot) => {
    setConfirming(null);
    try {
      await deletePilot(pilot);
      setError("");
    } catch (e) {
      console.error("pilot delete failed", e);
      setError(`Could not delete ${pilot.name}.`);
    }
    setPilots(listPilots());
    // The focused row is gone; keep focus inside the dialog so Escape still works.
    setTimeout(() => {
      const next = panel.current && panel.current.querySelector(".nova-row, .nova-actions button");
      if (next) next.focus();
    }, 0);
  };

  const exportOne = (pilot) => {
    try {
      exportPilot(pilot);
      setError("");
    } catch (e) {
      console.error("pilot export failed", e);
      setError(`Could not export ${pilot.name}.`);
    }
  };

  // Escape backs out of a pending delete before it closes the dialog.
  const escape = () => (confirming ? setConfirming(null) : onCancel());

  return html`
    <${Modal} title="Open Pilot" onCancel=${escape} panelRef=${panel}>
      <div class="nova-list">
        ${pilots.length === 0 && html`<div class="nova-empty">No saved pilots in this browser yet.</div>`}
        ${pilots.map((p) => html`
          <${PilotRow}
            key=${p.file}
            pilot=${p}
            confirming=${confirming === p.file}
            onPick=${() => onPick(p.path)}
            onExport=${() => exportOne(p)}
            onAskDelete=${() => setConfirming(p.file)}
            onDelete=${() => remove(p)}
            onKeep=${() => setConfirming(null)} />`)}
      </div>
      ${error && html`<div class="nova-error" role="alert">${error}</div>`}
      <div class="nova-actions">
        <button onClick=${onImport}>Import from file…</button>
        <button onClick=${onCancel}>Cancel</button>
      </div>
    <//>`;
}
