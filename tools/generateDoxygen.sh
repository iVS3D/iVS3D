#!/bin/bash
set -e

# Get the directory of the script
SCRIPT_DIR=$(dirname "$(realpath "$0")")
echo "Script directory: $SCRIPT_DIR"

cd $SCRIPT_DIR/..



# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Doxygen is not installed. Please install it to generate documentation."
    exit 1
fi

# Get the latest tag
VERSION=$(git describe --tags --abbrev=0)
echo "Generating docs for version $VERSION"

# Use a template Doxyfile.in and substitute @PROJECT_VERSION@
sed "s/@PROJECT_VERSION@/$VERSION/" $SCRIPT_DIR/templates/Doxyfile.template > Doxyfile

# Run doxygen
doxygen Doxyfile

echo "Documentation generated successfully."
