# minimalardupilot

This repository provides a lightweight, minimal mirror of the
official [ArduPilot](https://github.com/ArduPilot/ardupilot) source code.

Instead of downloading gigabytes of extensive Git history, this repository automatically mirrors official release tags (
such as `Plane-4.x.x`, `Copter-4.x.x`, etc.) as **single-commit branches**. Furthermore, it parses the `.gitmodules` for
each tag and mirrors the exact required submodule commits as standalone branches (e.g., `Plane-4.6.3-waf`).

This is ideal for developers, CI/CD pipelines, or deployment environments that need to pull the ArduPilot source code
and its dependencies as quickly and efficiently as possible.

## Collaboration & Cloning

To clone this repository for collaboration (specifically targeting the generator scripts on the `main` branch), use:

```bash
git clone -b main --single-branch git@github.com:larvance/minimalardupilot.git
```

## Running the Auto-Updater Daemon

The `main` branch contains a Python daemon (`main.py`) that checks the upstream ArduPilot repository hourly, generates
these single-commit branches for new tags, and pushes them here.

To set up the automated daemon on a Linux server:

1. Clone the repository using an authenticated URL (so the daemon can push).
2. Run the setup script as root to create the `systemd` service:

```bash
sudo ./setup.sh
```
