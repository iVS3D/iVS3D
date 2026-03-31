# Doxygen generation (local + CI)

This repository uses `tools/generateDoxygen.sh` to generate documentation.

## What the script does

1. Creates a temporary staging directory for markdown files.
2. Copies `README.md` and `doc/*.md` files into staging.
3. Uses Mermaid CLI (`mmdc`) to transform markdown Mermaid code blocks into SVG image references.
4. Runs Doxygen with staged markdown input so source docs stay unchanged.
5. Applies HTML post-processing in `generated_doc/html`.

## Required tools (local)

- `doxygen`
- `graphviz`
- `python3`
- `node` + `npm`
- `@mermaid-js/mermaid-cli` (provides `mmdc`)

Install Mermaid CLI (same version as CI):

```bash
npm install -g @mermaid-js/mermaid-cli@11.12.0
```

## Generate docs locally

Run from the `iVS3D` directory:

```bash
tools/generateDoxygen.sh
```

Output is written to `generated_doc/html`.

## CI note

The GitHub Pages workflow installs:

- Doxygen + Graphviz via `apt`
- Node.js (v20)
- Mermaid CLI pinned to `11.12.0`

Then it runs `tools/generateDoxygen.sh`.
