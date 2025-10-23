 
#!/bin/bash

# Get the script's directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Path to the installed binary (produced by install.sh)
APP_PATH="$SCRIPT_DIR/built/bin/epochai"

if [ ! -x "$APP_PATH" ]; then
  echo "Could not find executable at $APP_PATH"
  echo "Please run ./build.sh <compiler> <config> followed by ./install.sh <compiler> <config>."
  exit 1
fi

"$APP_PATH" "$@"
