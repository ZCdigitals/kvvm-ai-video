#!/bin/bash

VERSION_PATH="src/version.h"

bump_major() {
    current=$(grep "VERSION_MAJOR" $VERSION_PATH | grep -oE '[0-9]+')
    new=$((current + 1))
    sed -i "s/VERSION_MAJOR $current/VERSION_MAJOR $new/" $VERSION_PATH
    sed -i "s/VERSION_MINOR [0-9]*/VERSION_MINOR 0/" $VERSION_PATH
    sed -i "s/VERSION_PATCH [0-9]*/VERSION_PATCH 0/" $VERSION_PATH
}

bump_minor() {
    current=$(grep "VERSION_MINOR" $VERSION_PATH | grep -oE '[0-9]+')
    new=$((current + 1))
    sed -i "s/VERSION_MINOR $current/VERSION_MINOR $new/" $VERSION_PATH
    sed -i "s/VERSION_PATCH [0-9]*/VERSION_PATCH 0/" $VERSION_PATH
}

bump_patch() {
    current=$(grep "VERSION_PATCH" $VERSION_PATH | grep -oE '[0-9]+')
    new=$((current + 1))
    sed -i "s/VERSION_PATCH $current/VERSION_PATCH $new/" $VERSION_PATH
}

create_tag() {
    version=$(grep -E "VERSION_[A-Z]+ [0-9]+" $VERSION_PATH | awk '{printf("%s.", $3)}' | sed 's/.$//')
    tag="v$version"
    git tag -a "$tag" -m "Version $version"
}

if [$# -eq 1]; then
    case "$1" in
        "major")
            bump_major
            ;;
        "minor")
            bump_minor
            ;;
        "patch")
            bump_patch
            ;;
        *)
            echo "Unknwo args"
            exit 1
    esac
else
    bump_patch
fi

create_tag
