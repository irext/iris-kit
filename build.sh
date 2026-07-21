#!/bin/bash

echo "=========================================="
echo "IRIS Kit Firmware Build Script"
echo "=========================================="
echo ""
echo "Available commands:"
echo "  1. Build IREXT version"
echo "  2. Build GeekLink version"
echo "  3. Upload IREXT version"
echo "  4. Upload GeekLink version"
echo "  5. Clean all builds"
echo "  6. Exit"
echo ""

read -p "Enter your choice (1-6): " choice

case $choice in
    1)
        echo "Building IREXT version..."
        pio run -e esp8266-1m-irext
        ;;
    2)
        echo "Building GeekLink version..."
        pio run -e esp8266-1m-geeklink
        ;;
    3)
        echo "Uploading IREXT version..."
        pio run -e esp8266-1m-irext -t upload
        ;;
    4)
        echo "Uploading GeekLink version..."
        pio run -e esp8266-1m-geeklink -t upload
        ;;
    5)
        echo "Cleaning all builds..."
        pio run -t clean
        ;;
    6)
        echo "Exiting..."
        exit 0
        ;;
    *)
        echo "Invalid choice!"
        ;;
esac
