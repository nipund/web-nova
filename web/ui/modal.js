import { html, useEffect, useRef } from "./deps.js";

// Overlay + gunmetal panel with a title bar. Focuses the first button on open,
// closes on Escape, and keeps keystrokes away from the game's key handlers.
export function Modal({ title, onCancel, children }) {
  const panel = useRef(null);
  useEffect(() => {
    const first = panel.current && panel.current.querySelector("button");
    if (first) first.focus();
  }, []);

  // Capture phase: stop the event before it can bubble up to the game's listeners.
  const trap = (e) => {
    e.stopPropagation();
    if (e.type === "keydown" && e.key === "Escape") onCancel();
  };

  return html`
    <div class="nova-overlay nova-modal" onKeyDownCapture=${trap} onKeyUpCapture=${trap}>
      <div class="nova-panel" ref=${panel}>
        <div class="nova-title">${title}</div>
        ${children}
      </div>
    </div>`;
}
