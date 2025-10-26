echo

# Check if g++ exists
if command -v g++ &> /dev/null; then
    exit 0
fi

# echo "g++ not found. Installing..."
echo "You will be prompted for your password."
echo

if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    # echo "ERROR: Cannot detect Linux distribution"
    exit 1
fi

case $OS in
    ubuntu|debian)
        echo "Installing on Ubuntu/Debian..."
        sudo apt-get update
        sudo apt-get install -y g++ make
        ;;
    fedora)
        echo "Installing on Fedora..."
        sudo dnf install -y gcc-c++ make
        ;;
    centos|rhel)
        echo "Installing on CentOS/RHEL..."
        sudo yum install -y gcc-c++ make
        ;;
    arch)
        echo "Installing on Arch Linux..."
        sudo pacman -S --noconfirm gcc make
        ;;
    *)
        echo "ERROR: Unsupported distribution: $OS"
        echo "Please install g++ manually"
        exit 1
        ;;
esac

# Verify installation
if command -v g++ &> /dev/null; then
    echo
    echo "✓ Installation successful!"
    g++ --version
else
    echo
    echo "✗ Installation failed!"
    exit 1
fi
chmod +x install_deps.sh