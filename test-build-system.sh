#!/bin/bash

# * Project: OpenAuto
# * This file is part of openauto project.
# * Copyright (C) 2025 OpenCarDev Team
# *
# *  openauto is free software: you can redistribute it and/or modify
# *  it under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 3 of the License, or
# *  (at your option) any later version.
# *
# *  openauto is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with openauto. If not, see <http://www.gnu.org/licenses/>.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  AASDK Build System Test${NC}"
    echo -e "${BLUE}================================================${NC}"
}

print_step() {
    echo -e "${YELLOW}🔄 $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

test_docker_availability() {
    print_step "Testing Docker availability..."
    
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed"
        return 1
    fi
    
    if ! docker --version &> /dev/null; then
        print_error "Docker is not running"
        return 1
    fi
    
    print_success "Docker is available"
}

test_buildx_support() {
    print_step "Testing Docker Buildx support..."
    
    if ! docker buildx version &> /dev/null; then
        print_error "Docker Buildx is not available"
        return 1
    fi
    
    print_success "Docker Buildx is available"
}

test_multi_arch_support() {
    print_step "Testing multi-architecture support..."
    
    # Check if we can list supported platforms
    if ! docker buildx ls | grep -q "linux/arm64"; then
        print_error "ARM64 platform support not detected"
        return 1
    fi
    
    print_success "Multi-architecture support detected"
}

test_build_script() {
    print_step "Testing build script..."
    
    if [ ! -f "build.sh" ]; then
        print_error "build.sh not found"
        return 1
    fi
    
    if [ ! -x "build.sh" ]; then
        print_error "build.sh is not executable"
        return 1
    fi
    
    # Test help output
    if ! ./build.sh --help &> /dev/null; then
        print_error "build.sh help command failed"
        return 1
    fi
    
    print_success "Build script is functional"
}

test_dockerfile() {
    print_step "Testing Dockerfile..."
    
    if [ ! -f "Dockerfile" ]; then
        print_error "Dockerfile not found"
        return 1
    fi
    
    # Basic syntax check
    if ! docker buildx build --dry-run . &> /dev/null; then
        print_error "Dockerfile syntax check failed"
        return 1
    fi
    
    print_success "Dockerfile syntax is valid"
}

test_docker_compose() {
    print_step "Testing Docker Compose configuration..."
    
    if [ ! -f "docker-compose.yml" ]; then
        print_error "docker-compose.yml not found"
        return 1
    fi
    
    # Check if docker-compose is available (optional)
    if command -v docker-compose &> /dev/null; then
        if ! docker-compose config &> /dev/null; then
            print_error "docker-compose.yml syntax check failed"
            return 1
        fi
        print_success "Docker Compose configuration is valid"
    else
        print_step "Docker Compose not available, skipping validation"
    fi
}

test_github_workflow() {
    print_step "Testing GitHub workflow..."
    
    if [ ! -f ".github/workflows/build_native.yml" ]; then
        print_error "GitHub workflow not found"
        return 1
    fi
    
    # Basic YAML syntax check (if yq is available)
    if command -v yq &> /dev/null; then
        if ! yq eval '.jobs.build.strategy.matrix' .github/workflows/build_native.yml &> /dev/null; then
            print_error "GitHub workflow syntax check failed"
            return 1
        fi
    fi
    
    print_success "GitHub workflow is present"
}

run_minimal_build_test() {
    print_step "Running minimal build test (AMD64 only)..."
    
    # Create a minimal test build to verify the system works
    if ! timeout 300 docker buildx build \
        --platform linux/amd64 \
        --build-arg TARGET_ARCH=amd64 \
        --target base \
        --tag aasdk-test:amd64 \
        . &> /dev/null; then
        print_error "Minimal build test failed"
        return 1
    fi
    
    # Clean up test image
    docker rmi aasdk-test:amd64 &> /dev/null || true
    
    print_success "Minimal build test passed"
}

# Main test execution
main() {
    print_header
    
    local failed_tests=0
    
    # Run all tests
    test_docker_availability || ((failed_tests++))
    test_buildx_support || ((failed_tests++))
    test_multi_arch_support || ((failed_tests++))
    test_build_script || ((failed_tests++))
    test_dockerfile || ((failed_tests++))
    test_docker_compose || ((failed_tests++))
    test_github_workflow || ((failed_tests++))
    
    # Optional: Run minimal build test if requested
    if [ "$1" = "--build-test" ]; then
        run_minimal_build_test || ((failed_tests++))
    fi
    
    echo
    if [ $failed_tests -eq 0 ]; then
        print_success "All tests passed! Build system is ready."
        echo
        echo -e "${BLUE}Next steps:${NC}"
        echo "  • Run a test build: ./docker-build.sh amd64"
        echo "  • Build all architectures: ./docker-build.sh all"
        echo "  • Check GitHub Actions for automated builds"
        exit 0
    else
        print_error "$failed_tests test(s) failed"
        echo
        echo -e "${YELLOW}Please resolve the issues above before using the build system.${NC}"
        exit 1
    fi
}

# Show usage if requested
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "AASDK Build System Test Script"
    echo
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "OPTIONS:"
    echo "  --build-test    Also run a minimal build test (requires Docker)"
    echo "  --help, -h      Show this help message"
    echo
    echo "This script validates that the AASDK build system is properly configured."
    exit 0
fi

main "$@"