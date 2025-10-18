#!/bin/bash
echo "Setting up Python virtual environment..."

# Check if Python is available
if ! command -v python3 &> /dev/null
then
    echo "Error: python3 is not found. Please install Python 3."
    exit 1
fi

# Create virtual environment in .venv directory if it doesn't exist
if [ ! -d ".venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv .venv
fi

# Activate the virtual environment and install dependencies
echo "Installing dependencies from Scripts/requirements.txt..."
source .venv/bin/activate
pip install -r Scripts/requirements.txt

echo ""
echo "Setup complete. You can now configure the CMake project."
