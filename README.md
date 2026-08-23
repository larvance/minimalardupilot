# minimalardupilot

This repository provides a lightweight, minimal mirror of the official [ArduPilot](https://github.com/ArduPilot/ardupilot) source code.

Instead of downloading gigabytes of extensive Git history, this repository automatically mirrors official release tags (such as `Plane-4.x.x`, `Copter-4.x.x`, etc.) as **single-commit branches**. All submodule dependencies are **recursively inlined** directly into the release tree, eliminating the need for separate submodule initialization (`git submodule update --init --recursive`) or separate submodule branches.

This is ideal for developers, CI/CD pipelines, or deployment environments that need to pull complete ArduPilot release builds as quickly and efficiently as possible without needing extra submodule clone steps.

## Usage: Pulling Release Branches into an Existing Repository

You can add this repository as a secondary remote (e.g., `minimalap`) in your existing Git repository to quickly pull any fully inlined release branch (such as `Plane-4.6.3` or `Copter-4.5.7`) without downloading unnecessary Git history:

```bash
# 1. Add minimalardupilot as a remote named 'minimalap'
git remote add minimalap https://github.com/larvance/minimalardupilot.git

# 2. Fetch the specific release branch
git fetch minimalap Plane-4.6.3

# 3. Create a local branch from the fetched release
git checkout -b Plane-4.6.3 minimalap/Plane-4.6.3
```

### Direct Single-Branch Clone

To clone a single release tag directly into a standalone folder:

```bash
git clone --depth 1 -b Plane-4.6.3 https://github.com/larvance/minimalardupilot.git
```

## Collaboration & Maintenance

To clone this repository to work on the generator script (`main.py` on the `main` branch):

```bash
git clone -b main --single-branch git@github.com:larvance/minimalardupilot.git
```

## Running the Auto-Updater Daemon

The `main` branch contains a Python daemon (`main.py`) that checks the upstream ArduPilot repository hourly, generates single-commit branches for new tags with inlined submodules, and pushes them here.

To set up the automated daemon on a Linux server:

1. Clone the repository using an authenticated URL (so the daemon can push).
2. Run the setup script as root to create the `systemd` service:

```bash
sudo ./setup.sh
```
