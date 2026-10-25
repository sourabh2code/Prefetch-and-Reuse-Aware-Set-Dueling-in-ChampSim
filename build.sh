
#!/usr/bin/env bash
set -euo pipefail

# Usage:
# ./build.sh <LLC_PREFETCHER> <LLC_REPLACEMENT> <BINARY_NAME>
#
# Example:
# ./build.sh next_line drrip baseline_nextline

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <LLC_PREFETCHER> <LLC_REPLACEMENT> <BINARY_NAME>"
    echo "Example: $0 next_line drrip baseline_nextline"
    exit 1
fi

LLC_PREFETCHER="$1"
LLC_REPLACEMENT="$2"
BINARY_NAME="$3"

# Run from ChampSim root
cd "$(dirname "$0")"

# Validate argument names
for value in "$LLC_PREFETCHER" "$LLC_REPLACEMENT" "$BINARY_NAME"; do
    if [[ ! "$value" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        echo "Error: invalid name: $value"
        exit 1
    fi
done

# Validate LLC prefetcher
if [[ "$LLC_PREFETCHER" != "no" && ! -d "prefetcher/$LLC_PREFETCHER" ]]; then
    echo "Error: unknown prefetcher: $LLC_PREFETCHER"
    exit 1
fi

# Validate LLC replacement policy
if [[ ! -d "replacement/$LLC_REPLACEMENT" ]]; then
    echo "Error: unknown replacement policy: $LLC_REPLACEMENT"
    exit 1
fi

# Create config directory
mkdir -p configs

CONFIG="configs/${BINARY_NAME}.json"

# Prevent accidental overwriting of a different experiment
if [[ -e "$CONFIG" ]]; then
    echo "Configuration already exists: $CONFIG"
    echo "Use a different binary name or remove the old config."
    exit 1
fi

# Generate ChampSim configuration
cat > "$CONFIG" <<EOF
{
  "executable_name": "$BINARY_NAME",
  "L1D": {
    "prefetcher": "no"
  },
  "L2C": {
    "prefetcher": "no"
  },
  "LLC": {
    "prefetcher": "$LLC_PREFETCHER",
    "replacement": "$LLC_REPLACEMENT"
  }
}
EOF

echo "====================================="
echo "         ChampSim Build"
echo "====================================="
echo "L1D prefetcher  : no"
echo "L2C prefetcher  : no"
echo "LLC prefetcher  : $LLC_PREFETCHER"
echo "LLC replacement : $LLC_REPLACEMENT"
echo "Binary name     : $BINARY_NAME"
echo "====================================="

# Configure and compile
./config.sh "$CONFIG"
make -j4

# Verify binary
if [[ ! -f "bin/$BINARY_NAME" ]]; then
    echo "Error: binary not found!"
    exit 1
fi

echo ""
echo "Build successful!"
echo "Executable: bin/$BINARY_NAME"
