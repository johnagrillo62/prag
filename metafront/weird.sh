#!/bin/bash
# cleanup_weird_files.sh - Remove files with trailing dots

echo "Cleaning up weird files..."

# Remove files ending with just a dot
find . -name "*." -type f -print -delete

# Remove from git if tracked
git rm --cached "*.." "demo*." 2>/dev/null || true

echo "Cleanup complete!"


