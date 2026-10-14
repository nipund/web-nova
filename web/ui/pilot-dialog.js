import { html } from "./deps.js";
import { Modal } from "./modal.js";

export function PilotDialog({ pilots, onPick, onImport, onCancel }) {
  return html`
    <${Modal} title="Open Pilot" onCancel=${onCancel}>
      <div class="nova-list">
        ${pilots.length === 0 && html`<div class="nova-empty">No saved pilots in this browser yet.</div>`}
        ${pilots.map((p) => html`
          <button class="nova-row" key=${p.file} onClick=${() => onPick(p.path)}>${p.name}</button>`)}
      </div>
      <div class="nova-actions">
        <button onClick=${onImport}>Import from file…</button>
        <button onClick=${onCancel}>Cancel</button>
      </div>
    <//>`;
}
