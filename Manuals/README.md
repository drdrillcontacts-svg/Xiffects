# Xiffects Live user manuals

- `Xiffects-Live-User-Manual-FA.pdf`: seven-page Persian guide; embedded font, per-line Arabic shaping and explicit RTL base direction for mixed Persian/English paragraphs.
- `Xiffects-Live-User-Manual-EN.pdf`: seven-page English guide.
- `Xiffects-Live-User-Manuals.zip`: both PDFs together.

Editable bilingual content: `content.json`. Rebuild from the repository root:

```
python -m venv .venv
.venv/bin/pip install -r Manuals/requirements.txt
.venv/bin/python Tools/BuildManuals.py
```

The builder uses the system DejaVu Sans font at `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`; adjust `FONT` for another platform. It embeds a subset in the PDF. Content targets interface revision `90b6ecf`. PDF generation checks page overflow; final QA renders all 14 pages and checks all text bounds. Persian mixed-direction lines were visually reviewed after fixing their base direction.
