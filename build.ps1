# AASDK Build Script for Windows PowerShell
# This PowerShell script provides Windows-native building capabilities

param(
    [Parameter(Position=0)]
    [ValidateSet("debug", "release")]
    [string]$BuildType = "debug",
    
    [Parameter(Position=1, ValueFromRemainingArguments=$true)]
    [string[]]$Options = @(),
    
    [string]$TargetArch = "amd64",
    [int]$Jobs = (Get-CimInstance -ClassName Win32_ComputerSystem).NumberOfLogicalProcessors,
    [string]$CMakeArgs = "",
    [switch]$Help
)

# Colors for output
$colors = @{
    Red = "Red"
    Green = "Green" 
    Yellow = "Yellow"
    Blue = "Blue"
    White = "White"
}

function Write-ColorOutput {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $colors[$Color]
}

function Write-Header {
    Write-ColorOutput "================================================" "Blue"
    Write-ColorOutput "  AASDK Build Script (PowerShell)" "Blue"
    Write-ColorOutput "================================================" "Blue"
    Write-ColorOutput "Build Type:     $BuildType" "Green"
    Write-ColorOutput "Architecture:   $TargetArch" "Green"
    Write-ColorOutput "Parallel Jobs:  $Jobs" "Green"
    Write-ColorOutput "Options:        $($Options -join ', ')" "Green"
    Write-ColorOutput "================================================" "Blue"
    Write-Host
}

function Write-Step {
    param([string]$Message)
    Write-ColorOutput "🔄 $Message" "Yellow"
}

function Write-Success {
    param([string]$Message)
    Write-ColorOutput "✅ $Message" "Green"
}

function Write-Error {
    param([string]$Message)
    Write-ColorOutput "❌ $Message" "Red"
}

function Show-Usage {
    Write-Host @"
AASDK Build Script for Windows PowerShell

Usage: .\build.ps1 [BuildType] [Options]

Parameters:
  -BuildType      Build configuration (debug, release) [default: debug]
  -TargetArch     Target architecture (amd64, x86) [default: amd64]
  -Jobs           Number of parallel build jobs [default: CPU cores]
  -CMakeArgs      Additional CMake arguments
  -Help           Show this help message

Options (pass as additional arguments):
  clean           Clean build directory before building
  test            Run tests after building
  install         Install after building
  package         Create packages after building

Examples:
  .\build.ps1 debug
  .\build.ps1 release clean
  .\build.ps1 -BuildType debug -TargetArch amd64 clean test
  .\build.ps1 release -Jobs 4

Environment Requirements:
  This script requires a Unix-like environment for the actual build process.
  
Recommended setup:
  1. VS Code DevContainers (recommended)
     - Provides complete isolated build environment
     - Supports cross-compilation for ARM architectures
  
  2. Windows Subsystem for Linux (WSL)
     - Install: wsl --install
     - Navigate to project in WSL and use ./build.sh
  
  3. Git Bash / MSYS2
     - Use bash version of the build script

For complete documentation, see BUILD.md
"@
}

function Test-BuildEnvironment {
    Write-Step "Checking build environment..."
    
    # Check for WSL
    if (Get-Command wsl -ErrorAction SilentlyContinue) {
        Write-ColorOutput "WSL is available" "Green"
        return "wsl"
    }
    
    # Check for bash (Git Bash, MSYS2, etc.)
    if (Get-Command bash -ErrorAction SilentlyContinue) {
        Write-ColorOutput "Bash is available" "Green"
        return "bash"
    }
    
    # Check for Docker (DevContainer support)
    if (Get-Command docker -ErrorAction SilentlyContinue) {
        Write-ColorOutput "Docker is available - DevContainer recommended" "Yellow"
        Write-ColorOutput "To use DevContainer:" "Yellow"
        Write-ColorOutput "  1. Open project in VS Code" "Yellow"
        Write-ColorOutput "  2. Install Dev Containers extension" "Yellow"
        Write-ColorOutput "  3. Ctrl+Shift+P > 'Dev Containers: Reopen in Container'" "Yellow"
        return "docker"
    }
    
    return $null
}

function Invoke-UnixBuild {
    param([string]$Environment)
    
    $buildArgs = @($BuildType) + $Options
    $argString = $buildArgs -join " "
    
    switch ($Environment) {
        "wsl" {
            Write-Step "Building using WSL..."
            $env:TARGET_ARCH = $TargetArch
            $env:JOBS = $Jobs
            $env:CMAKE_ARGS = $CMakeArgs
            
            wsl bash -c "cd '$PWD' && ./build.sh $argString"
        }
        "bash" {
            Write-Step "Building using Bash..."
            $env:TARGET_ARCH = $TargetArch
            $env:JOBS = $Jobs
            $env:CMAKE_ARGS = $CMakeArgs
            
            bash -c "./build.sh $argString"
        }
        default {
            Write-Error "No suitable build environment found"
            Write-Host @"

This project requires a Unix-like environment to build. 

Recommended solutions:

1. VS Code DevContainers (Recommended)
   - Install Docker Desktop
   - Install VS Code with Dev Containers extension
   - Open this project in VS Code
   - Press Ctrl+Shift+P > "Dev Containers: Reopen in Container"
   - Use ./build.sh inside the container

2. Windows Subsystem for Linux (WSL)
   - Install WSL: wsl --install
   - Navigate to this directory in WSL: cd /mnt/c/path/to/aasdk
   - Run: ./build.sh $argString

3. Git Bash
   - Install Git for Windows (includes Git Bash)
   - Open Git Bash in this directory
   - Run: ./build.sh $argString

For complete setup instructions, see BUILD.md
"@
            exit 1
        }
    }
}

function Show-BuildComplete {
    Write-Host
    Write-ColorOutput "================================================" "Blue"
    Write-ColorOutput "🎉 AASDK Build Process Completed!" "Green"
    Write-ColorOutput "================================================" "Blue"
    Write-Host
    Write-ColorOutput "For detailed build output and artifacts, check the build environment." "Yellow"
    Write-ColorOutput "Build artifacts are typically located in:" "Yellow"
    Write-ColorOutput "  • build-debug/ or build-release/ directories" "Yellow"
    Write-ColorOutput "  • packages/ directory (if packages were created)" "Yellow"
    Write-Host
    Write-ColorOutput "Next steps:" "Yellow"
    Write-ColorOutput "  • Check build logs for any issues" "Yellow"
    Write-ColorOutput "  • Run tests: cd build-debug && ctest" "Yellow"
    Write-ColorOutput "  • See TROUBLESHOOTING.md for common issues" "Yellow"
    Write-ColorOutput "================================================" "Blue"
}

# Main execution
if ($Help) {
    Show-Usage
    exit 0
}

Write-Header

$buildEnv = Test-BuildEnvironment

if ($buildEnv) {
    try {
        Invoke-UnixBuild $buildEnv
        Show-BuildComplete
    }
    catch {
        Write-Error "Build process failed: $_"
        Write-ColorOutput "Check the error output above and see TROUBLESHOOTING.md for solutions." "Yellow"
        exit 1
    }
}
else {
    Write-Error "No suitable build environment detected"
    exit 1
}
