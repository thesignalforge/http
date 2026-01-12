# Signalforge HTTP Extension - Docker-based Build
IMAGE_NAME = signalforge-http
VERSION ?= 85

.PHONY: docker-build docker-test docker-example docker-shell docker-clean valgrind-test valgrind-docker ci-test-all test-version build-push help

help:
	@echo "Signalforge HTTP Extension"
	@echo ""
	@echo "Usage:"
	@echo "  make docker-build     - Build Docker image with extension (default: PHP 8.5, VERSION=85)"
	@echo "  make docker-test      - Run tests in Docker (default: PHP 8.5, VERSION=85)"
	@echo "  make docker-example   - Run example in Docker"
	@echo "  make docker-shell     - Interactive shell in Docker"
	@echo "  make docker-clean     - Remove Docker images"
	@echo "  make ci-test-all      - Build and test PHP 8.3, 8.4, and 8.5"
	@echo "  make test-version     - Run tests using locally built extension (VERSION=85)"
	@echo "  make valgrind-docker  - Run Valgrind memory check in Docker"
	@echo "  make valgrind-test    - Run tests with local Valgrind"
	@echo ""
	@echo "Supported versions: VERSION=83 (PHP 8.3), VERSION=84 (PHP 8.4), VERSION=85 (PHP 8.5)"
	@echo "Example: make docker-build VERSION=84"

docker-build:
	docker build --platform linux/amd64 --build-arg VERSION=$(VERSION) -t $(IMAGE_NAME):$(VERSION) -t $(IMAGE_NAME):latest .

docker-test:
	docker run --rm -v $(PWD)/tests:/ext/tests $(IMAGE_NAME):$(VERSION) php /opt/run-tests.php /ext/tests/

docker-example:
	docker run --rm -v $(PWD)/examples:/ext/examples $(IMAGE_NAME):$(VERSION) php /ext/examples/basic.php

docker-shell:
	docker run --rm -it -v $(PWD):/ext $(IMAGE_NAME):$(VERSION) sh

docker-clean:
	docker rmi $(IMAGE_NAME) 2>/dev/null || true

valgrind-test:
	@echo "Running tests with Valgrind memory leak detection..."
	@echo "This may take several minutes..."
	@valgrind --version > /dev/null 2>&1 || (echo "Error: Valgrind not installed. Install with: apt install valgrind (Linux) or brew install valgrind (macOS)" && exit 1)
	@valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--verbose \
		--log-file=valgrind-output.txt \
		php /opt/run-tests.php tests/ || true
	@echo ""
	@echo "Valgrind output saved to: valgrind-output.txt"
	@echo ""
	@grep -A 5 "LEAK SUMMARY" valgrind-output.txt || echo "No leaks detected!"
	@grep "ERROR SUMMARY" valgrind-output.txt || echo "No errors detected!"

# CI: Build and test all PHP versions
ci-test-all:
	@echo "Building and testing PHP 8.3..."
	docker build --platform linux/amd64 --build-arg VERSION=83 -t $(IMAGE_NAME):83 .
	docker run --rm -v $(PWD)/tests:/ext/tests $(IMAGE_NAME):83 php /opt/run-tests.php -q /ext/tests/
	@echo ""
	@echo "Building and testing PHP 8.4..."
	docker build --platform linux/amd64 --build-arg VERSION=84 -t $(IMAGE_NAME):84 .
	docker run --rm -v $(PWD)/tests:/ext/tests $(IMAGE_NAME):84 php /opt/run-tests.php -q /ext/tests/
	@echo ""
	@echo "Building and testing PHP 8.5..."
	docker build --platform linux/amd64 --build-arg VERSION=85 -t $(IMAGE_NAME):85 .
	docker run --rm -v $(PWD)/tests:/ext/tests $(IMAGE_NAME):85 php /opt/run-tests.php -q /ext/tests/
	@echo ""
	@echo "All PHP versions tested successfully!"

# Valgrind: Run memory check in Docker
valgrind-docker:
	@echo "Building Valgrind Docker image..."
	docker build --platform linux/amd64 -f Dockerfile.valgrind -t $(IMAGE_NAME):valgrind .
	@echo ""
	@echo "Running Valgrind memory check..."
	docker run --rm -v $(PWD)/tests:/ext/tests -v $(PWD):/output $(IMAGE_NAME):valgrind \
		sh -c 'valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 \
		       --track-origins=yes --log-file=/output/valgrind-output.txt \
		       php /opt/run-tests.php -q /ext/tests/ && \
		       cat /output/valgrind-output.txt | grep -E "ERROR SUMMARY|LEAK SUMMARY" -A 5'

# Run tests using locally built extension with configurable PHP version
test-version:
	@echo "Running tests with locally built PHP 8.$(VERSION) extension (linux/amd64)..."
	docker run --rm --platform linux/amd64 -v $(PWD)/tests:/ext/tests $(IMAGE_NAME):$(VERSION) \
		php /opt/run-tests.php /ext/tests/
