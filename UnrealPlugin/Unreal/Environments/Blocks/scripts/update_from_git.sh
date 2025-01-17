#!/bin/bash

# get path of current script: https://stackoverflow.com/a/39340259/207661
#SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#pushd "$SCRIPT_DIR" >/dev/null

set -e
set -x

./scripts/clean.sh

rsync -a  --exclude 'temp' --delete ../../Plugins/AutonomySim Plugins/
rsync -a  --exclude 'temp' --delete ../../../AutonomyLib Plugins/AutonomySim/Source/

#popd >/dev/null