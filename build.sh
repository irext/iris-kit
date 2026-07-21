#!/bin/bash
# IRIS Kit Firmware Build Script
# Support compilation and upload for IREXT and GeekLink hardware versions

set -e

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored messages
print_info() {
    echo -e "${BLUE} $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Display main menu
show_menu() {
    echo ""
    echo "=========================================="
    echo -e "${GREEN}IRIS Kit Firmware Build Script${NC}"
    echo "=========================================="
    echo ""
    echo "Hardware Versions:"
    echo "  [1] IRIS_KIT_IREXT      -> iris_kit_latest.bin"
    echo "  [2] IRIS_KIT_GEEKLINK   -> iris_kit_geeklink_latest.bin"
    echo ""
    echo "Build Commands:"
    echo "  [3] Build IREXT version"
    echo "  [4] Build GeekLink version"
    echo "  [5] Build ALL versions"
    echo ""
    echo "Upload Commands:"
    echo "  [6] Upload IREXT version"
    echo "  [7] Upload GeekLink version"
    echo ""
    echo "Maintenance:"
    echo "  [8] Clean all builds"
    echo "  [9] Show build info"
    echo "  [0] Exit"
    echo ""
}

# Build firmware
build_firmware() {
    local env_name=$1
    local version_name=$2
    
    print_info "Building $version_name firmware..."
    if pio run -e "$env_name"; then
        print_success "$version_name build completed successfully!"
        echo ""
        print_info "Output file: .pio/build/$env_name/iris_kit_${version_name,,}_latest.bin"
    else
        print_error "$version_name build failed!"
        return 1
    fi
}

# Upload firmware
upload_firmware() {
    local env_name=$1
    local version_name=$2
    
    print_info "Uploading $version_name firmware..."
    if pio run -e "$env_name" -t upload; then
        print_success "$version_name firmware uploaded successfully!"
    else
        print_error "$version_name upload failed!"
        return 1
    fi
}

# Clean all builds
clean_all() {
    print_info "Cleaning all builds..."
    if pio run -t clean; then
        print_success "All builds cleaned!"
    else
        print_error "Clean failed!"
        return 1
    fi
}

# Show build info
show_build_info() {
    echo ""
    echo "=========================================="
    echo -e "${GREEN}Build Information${NC}"
    echo "=========================================="
    echo ""
    print_info "Available environments:"
    pio project config 2>&1 | grep "^env:" | sed 's/^/  /'
    echo ""
    print_info "Firmware files:"
    if [ -d ".pio/build" ]; then
        find .pio/build -name "*.bin" -type f 2>/dev/null | while read file; do
            size=$(du -h "$file" | cut -f1)
            echo "  [$size] $file"
        done
    else
        echo "  No firmware files found. Please build first."
    fi
    echo ""
}
# Main loop
while true; do
    show_menu
    read -p "Enter your choice [0-9]: " choice
    
    case $choice in
        1)
            echo ""
            print_info "Environment: esp8266-1m-irext"
            print_info "Macro: -DIRIS_KIT_IREXT=1"
            print_info "Output: iris_kit_latest.bin"
            echo ""
            ;;
        2)
            echo ""
            print_info "Environment: esp8266-1m-geeklink"
            print_info "Macro: -DIRIS_KIT_GEEKLINK=1"
            print_info "Output: iris_kit_geeklink_latest.bin"
            echo ""
            ;;
        3)
            build_firmware "esp8266-1m-irext" "IREXT"
            ;;
        4)
            build_firmware "esp8266-1m-geeklink" "GeekLink"
            ;;
        5)
            print_info "Building ALL versions..."
            echo ""
            build_firmware "esp8266-1m-irext" "IREXT"
            echo ""
            build_firmware "esp8266-1m-geeklink" "GeekLink"
            echo ""
            print_success "All versions built successfully!"
            ;;
        6)
            upload_firmware "esp8266-1m-irext" "IREXT"
            ;;
        7)
            upload_firmware "esp8266-1m-geeklink" "GeekLink"
            ;;
        8)
            clean_all
            ;;
        9)
            show_build_info
            ;;
        0)
            echo ""
            print_info "Goodbye!"
            exit 0
            ;;
        *)
            echo ""
            print_error "Invalid choice! Please enter a number between 0-9."
            ;;
    esac
    
    # Wait for user to press Enter to continue (except for exit)
    if [ "$choice" != "0" ]; then
        echo ""
        read -p "Press Enter to continue..."
    fi
done
