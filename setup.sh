#!/usr/bin/env bash
set -euo pipefail

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (or use sudo)"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_FILE="/etc/systemd/system/minimalardupilot.service"

echo "Ensuring main.py is executable..."
chmod +x "$SCRIPT_DIR/main.py"

echo "Creating systemd service at $SERVICE_FILE..."
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Minimal ArduPilot GitHub Auto-Updater Daemon
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$SCRIPT_DIR
ExecStart=/usr/bin/python3 $SCRIPT_DIR/main.py
Restart=always
RestartSec=10
Environment=PYTHONUNBUFFERED=1

[Install]
WantedBy=multi-user.target
EOF

echo "Reloading systemd daemon..."
systemctl daemon-reload

echo "Enabling and starting the service..."
systemctl enable minimalardupilot.service
systemctl restart minimalardupilot.service

echo "Setup complete!"
echo "Check the live logs at any time using: journalctl -fu minimalardupilot.service"
