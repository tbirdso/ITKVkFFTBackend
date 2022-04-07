#!/bin/bash

# Run this script to build the Python wheel packages for Linux for an ITK
# external module.
#
# Versions can be restricted by passing them in as arguments to the script
# For example,
#
#   scripts/dockcross-manylinux-build-module-wheels.sh cp39

# Generate dockcross scripts
docker run --rm dockcross/manylinux2014-x64:20201015-96d8741 > /tmp/dockcross-manylinux-x64
chmod u+x /tmp/dockcross-manylinux-x64

script_dir=$(cd $(dirname $0) || exit 1; pwd)

# TODO download CUDA dependencies

# Build wheels
mkdir -p dist
DOCKER_ARGS="-v $(pwd)/dist:/work/dist/ -v $(pwd)/ITKPythonPackage:/ITKPythonPackage -v $(pwd)/tools:/tools -v /usr/local/cuda:/cuda"
/tmp/dockcross-manylinux-x64 \
  -a "$DOCKER_ARGS" \
  "/ITKPythonPackage/scripts/internal/manylinux-build-module-wheels.sh" "$@"
