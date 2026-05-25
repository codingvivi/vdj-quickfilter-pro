root-dir := justfile_directory()


tools-dir := root-dir / "tools"

build-dir := root-dir / "build"

name := "QFPro"

bin := build-dir / name + ".so"
bundle :=  build-dir / name + ".bundle"
build-dest := "$HOME/Library/Application Support/VirtualDJ/PluginsMacArm/Autostart"

mac-host := "mbp-21-16"
mac-dir := "~/code/vdj-quickfilter-pro"

# show overview of recipes
help:
    just -l

# Configure build (idempotent; reconfigures if prefix changed)
[macos]
setup:
    if [ -f {{build-dir}}/build.ninja ]; then \
        meson setup --reconfigure {{build-dir}} \
            --prefix="{{build-dest}}/{{name}}.bundle" \
            --bindir=Contents/MacOS; \
    else \
        meson setup {{build-dir}} \
            --prefix="{{build-dest}}/{{name}}.bundle" \
            --bindir=Contents/MacOS; \
    fi

# Compile
[macos]
build: setup
    meson compile -C {{build-dir}}

# Install bundle to VDJ plugin directory
[macos]
install: build
    meson install -C {{build-dir}}

# Sync to Mac, build, bundle, and install
[linux]
deploy: sync
    ssh {{mac-host}} "cd {{mac-dir}} && /opt/homebrew/bin/just install"

# Sync source to Mac (excludes build artifacts, LSP cache, stubs)
[linux]
sync:
    rsync -avz --delete \
        --exclude='build/' \
        --exclude='tools/stubs/' \
        --exclude='.cache/' \
        --exclude='.git/' \
        --exclude='.jj/' \
        --exclude='compile_flags.txt' \
        ./ "{{mac-host}}:{{mac-dir}}/"
