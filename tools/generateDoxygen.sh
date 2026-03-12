#!/bin/bash
set -euo pipefail

# Get the directory of the script
SCRIPT_DIR=$(dirname "$(realpath "$0")")
echo "Script directory: $SCRIPT_DIR"

cd "$SCRIPT_DIR/.."
PROJECT_ROOT=$(pwd)

# Staging directory for markdown preprocessing (keeps source docs unchanged)
STAGING_DIR=$(mktemp -d "${TMPDIR:-/tmp}/ivs3d-doxygen-md.XXXXXX")
trap 'rm -rf "$STAGING_DIR"' EXIT

stage_markdown_sources() {
    mkdir -p "$STAGING_DIR/doc"
    cp "$PROJECT_ROOT/README.md" "$STAGING_DIR/README.md"
    cp -a "$PROJECT_ROOT/doc/." "$STAGING_DIR/doc/"
}

preprocess_mermaid_diagrams() {
    # Mermaid CLI is required for converting Mermaid code fences to SVG.
    if ! command -v mmdc &> /dev/null; then
        echo "Mermaid CLI (mmdc) is not installed. Install @mermaid-js/mermaid-cli to enable Mermaid rendering."
        exit 1
    fi

    local puppeteer_config="$STAGING_DIR/puppeteer-config.json"
    cat > "$puppeteer_config" <<'JSON'
{
  "args": ["--no-sandbox", "--disable-setuid-sandbox"]
}
JSON

    local processed_files=0
    local md_file
    while IFS= read -r -d '' md_file; do
        if ! grep -Eq '^```[[:space:]]*mermaid([[:space:]]|$)' "$md_file"; then
            continue
        fi

        echo "Rendering Mermaid diagrams in $md_file"
        mmdc \
            -i "$md_file" \
            -o "$md_file" \
            -t neutral \
            -b transparent \
            -p "$puppeteer_config"

        processed_files=$((processed_files + 1))
    done < <(
        {
            printf '%s\0' "$STAGING_DIR/README.md"
            find "$STAGING_DIR/doc" -type f -name '*.md' -print0
        }
    )

    echo "Mermaid preprocessing complete: transformed $processed_files markdown file(s)."
}

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Doxygen is not installed. Please install it to generate documentation."
    exit 1
fi

# Get the latest tag
VERSION=$(git describe --tags --abbrev=0)
echo "Generating docs for version $VERSION"

# Prepare staged markdown and render Mermaid diagrams as SVG files
stage_markdown_sources
preprocess_mermaid_diagrams

# Use a template Doxyfile.in and substitute @PROJECT_VERSION@
ESCAPED_VERSION=${VERSION//&/\\&}
sed "s|@PROJECT_VERSION@|$ESCAPED_VERSION|" "$SCRIPT_DIR/templates/Doxyfile.template" > Doxyfile

# Override markdown-related paths so Doxygen uses staged files only.
export DOXYFILE_PATH="$PROJECT_ROOT/Doxyfile"
export DOXY_INPUT_README="$STAGING_DIR/README.md"
export DOXY_INPUT_LICENSE="$PROJECT_ROOT/LICENSE"
export DOXY_INPUT_LOGO="$PROJECT_ROOT/iVS3D.png"
export DOXY_INPUT_SRC="$PROJECT_ROOT/iVS3D/src"
export DOXY_INPUT_DOC="$STAGING_DIR/doc"
export DOXY_MAINPAGE="$STAGING_DIR/README.md"
export DOXY_IMAGE_PATH_DOC="$STAGING_DIR/doc"
export DOXY_IMAGE_PATH_ROOT="$PROJECT_ROOT"
export DOXY_STRIP_PATH_STAGE="$STAGING_DIR"
export DOXY_STRIP_PATH_ROOT="$PROJECT_ROOT"

python3 <<'PY'
import os
import re
from pathlib import Path

doxyfile = Path(os.environ["DOXYFILE_PATH"])
content = doxyfile.read_text(encoding="utf-8").splitlines()

new_input_block = [
    f'INPUT                  = "{os.environ["DOXY_INPUT_README"]}" \\\\',
    f'                         "{os.environ["DOXY_INPUT_LICENSE"]}" \\\\',
    f'                         "{os.environ["DOXY_INPUT_LOGO"]}" \\\\',
    f'                         "{os.environ["DOXY_INPUT_SRC"]}" \\\\',
    f'                         "{os.environ["DOXY_INPUT_DOC"]}"',
]

new_lines = []
i = 0
while i < len(content):
    line = content[i]
    stripped_line = line.lstrip()

    if re.match(r"^INPUT\s*=", stripped_line):
        new_lines.extend(new_input_block)
        i += 1
        while i < len(content):
            next_line = content[i]
            stripped = next_line.lstrip()
            if not stripped:
                break
            if stripped.startswith("#"):
                break
            if "=" in stripped:
                break
            i += 1
        continue

    if re.match(r"^USE_MDFILE_AS_MAINPAGE\s*=", stripped_line):
        new_lines.append(f'USE_MDFILE_AS_MAINPAGE = "{os.environ["DOXY_MAINPAGE"]}"')
        i += 1
        continue

    if re.match(r"^IMAGE_PATH\s*=", stripped_line):
        new_lines.append(
            f'IMAGE_PATH             = "{os.environ["DOXY_IMAGE_PATH_DOC"]}" "{os.environ["DOXY_IMAGE_PATH_ROOT"]}"'
        )
        i += 1
        continue

    if re.match(r"^STRIP_FROM_PATH\s*=", stripped_line):
        new_lines.append(
            f'STRIP_FROM_PATH        = "{os.environ["DOXY_STRIP_PATH_STAGE"]}" "{os.environ["DOXY_STRIP_PATH_ROOT"]}"'
        )
        i += 1
        continue

    new_lines.append(line)
    i += 1

doxyfile.write_text("\n".join(new_lines) + "\n", encoding="utf-8")
PY

# Remove stale documentation artifacts from previous runs.
rm -rf "$PROJECT_ROOT/generated_doc"

# Run doxygen
doxygen Doxyfile

# Postprocessing
cd generated_doc/html

# Replace video asset url with html video tag in index.html
VIDEO_ASSET_URL="https://github.com/user-attachments/assets/1f0c93f8-5e52-4436-a95e-5b9e3b7ea11d"
VIDEO_SRC="https://github.com/iVS3D/iVS3D/raw/refs/heads/video/iVS3D.mp4"
INDEX_FILE="index.html"

if [ ! -f "$INDEX_FILE" ]; then
    echo "$INDEX_FILE not found, skipping video replacement."
else
    if grep -qF "$VIDEO_ASSET_URL" "$INDEX_FILE"; then
        perl -0777 -i -pe '
            BEGIN { $a = shift; $s = shift }
            $r = qq{<p><video width="100%" controls>\n  <source src="$s" type="video/mp4">\n  Your browser does not support the video tag.\n</video></p>};
            s#<p>\s*<a\s+href="\Q$a\E">[^<]*</a>\s*</p>#$r#g;
        ' "$VIDEO_ASSET_URL" "$VIDEO_SRC" "$INDEX_FILE"
        echo "Replaced GitHub asset link with video tag in $INDEX_FILE"
    else
        echo "Asset URL not found in $INDEX_FILE, no changes made."
    fi
fi



echo "Documentation generated successfully."

# Additional postprocessing: replace documentation badge and LICENSE link
INDEX_FILE="index.html"
DOC_OLD_HREF="https://ivs3d.github.io/iVS3D/"
DOC_OLD_IMG="https://img.shields.io/badge/Documentation-blue"
DOC_NEW_HREF="https://github.com/iVS3D/iVS3D"
DOC_NEW_IMG="https://img.shields.io/badge/GitHub-Repository-181717?logo=github"

if [ -f "$INDEX_FILE" ]; then
    if grep -qF "$DOC_OLD_HREF" "$INDEX_FILE" || grep -qF "$DOC_OLD_IMG" "$INDEX_FILE"; then
        perl -0777 -i -pe '
            BEGIN { $oldHref = shift; $oldImg = shift; $newHref = shift; $newImg = shift }
            s#<a\s+href="\Q$oldHref\E">\s*<img\s+src="\Q$oldImg\E"([^>]*)>\s*</a>#<a href="$newHref"><img src="$newImg"$1></a>#gs;
        ' "$DOC_OLD_HREF" "$DOC_OLD_IMG" "$DOC_NEW_HREF" "$DOC_NEW_IMG" "$INDEX_FILE"
        echo "Replaced documentation badge with GitHub repo shield in $INDEX_FILE"
    else
        echo "Documentation badge not found in $INDEX_FILE, no changes made."
    fi

    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    LICENSE_URL="https://github.com/iVS3D/iVS3D/blob/$BRANCH/LICENSE"
    if grep -qF "[LICENSE](LICENSE)" "$INDEX_FILE"; then
        perl -0777 -i -pe "s#\\[LICENSE\\]\\(LICENSE\\)#<a href=\"$LICENSE_URL\">LICENSE</a>#g" "$INDEX_FILE"
        echo "Replaced markdown license link with GitHub link in $INDEX_FILE"
    else
        echo "Markdown license link not found in $INDEX_FILE, no changes made."
    fi
fi

# Remove the standalone SVG + caption and the "Quick Start" header so only the
# video remains. The SVG will be used as the poster (thumbnail) by the video tag.
# This targets the anchor id generated by Doxygen for the quick start section.
if [ -f "$INDEX_FILE" ]; then
    perl -0777 -i -pe '
        # Remove the <div class="image"> ... </div> block followed by the
        # Quick Start <h2> with anchor id "autotoc_md1". Keep the following
        # video paragraph untouched (so the video remains and uses the SVG as poster).
        s#<div class="image">.*?</div>\s*<h2>\s*<a class="anchor" id="autotoc_md1".*?</h2>\s*##gs;
    ' "$INDEX_FILE"
    echo "Removed standalone SVG image and Quick Start header from $INDEX_FILE"
fi

# Ensure the <video> tag has a poster attribute (use the SVG as thumbnail)
VIDEO_POSTER="iVS3D_overview.svg"
if [ -f "$INDEX_FILE" ]; then
    if grep -qF "poster=" "$INDEX_FILE"; then
        echo "Video already has a poster attribute, skipping poster insertion."
    else
        perl -0777 -i -pe 'unless (/<video\b[^>]*\bposter=/) { s#(<video\b[^>]*?)>#\1 poster="'"$VIDEO_POSTER"'">#s }' "$INDEX_FILE"
        echo "Inserted poster=\"$VIDEO_POSTER\" into the first <video> tag in $INDEX_FILE"
    fi
fi

# Beautify GitLab-style checklist markers in the Future Work list
# Replace '[x]' (checked) with an emoji and '[ ]' or '[_]' with an empty box emoji
if [ -f "$INDEX_FILE" ]; then
    perl -0777 -i -pe '
        # Replace checked boxes [x] or [X] at start of list items with ✅
        s#(<li>\s*)\[\s*[xX]\s*\]#\1✅ #g;
        # Replace unchecked boxes [ ] (or []) with ⬜
        s#(<li>\s*)\[\s*\]\s*#\1⬜ #g;
        # Replace variants like [ ] with two spaces inside or [ - ] etc.
        s#(<li>\s*)\[\s*[-_]\s*\]#\1⬜ #g;
    ' "$INDEX_FILE"
    echo "Replaced GitLab checklist markers with emoji in $INDEX_FILE"
fi
