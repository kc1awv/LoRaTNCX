#!/bin/bash
# Installation script for TNC Terminal

echo "Setting up LoRa TNC Terminal..."

# Check if Python 3 is installed
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed. Please install Python 3 first."
    exit 1
fi

# Check if pyserial is available
echo "Checking for required Python packages..."
if python3 -c "import serial" 2>/dev/null; then
    echo "✓ pyserial is already installed"
else
    echo "Installing required Python packages..."
    
    # Try different installation methods
    if command -v pacman &> /dev/null; then
        echo "Detected Arch Linux - installing via pacman..."
        sudo pacman -S python-pyserial
    elif command -v apt &> /dev/null; then
        echo "Detected Debian/Ubuntu - installing via apt..."
        sudo apt install python3-serial
    elif command -v yum &> /dev/null; then
        echo "Detected RHEL/CentOS - installing via yum..."
        sudo yum install python3-pyserial
    else
        echo "Installing via pip (user)..."
        pip3 install --user pyserial
    fi
fi

# Make the terminal script executable
chmod +x tnc_terminal.py

echo "Installation complete!"
echo ""
echo "Usage:"
echo "  ./tnc_terminal.py /dev/ttyACM0"
echo "  ./tnc_terminal.py /dev/ttyUSB0 -b 9600"
echo ""
echo "Available serial ports:"
ls /dev/tty* 2>/dev/null | grep -E "(ACM|USB)" | head -10