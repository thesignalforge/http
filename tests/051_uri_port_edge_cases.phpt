--TEST--
signalforge_http: Uri port edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// Test removing port with null
$uri = Uri::fromString('http://example.com:8080/');
$modified = $uri->withPort(null);
echo "Port removed: ";
var_dump($modified->getPort());

// Test valid port range (1-65535)
$uri = Uri::fromString('http://example.com/');

// Test port 1
$modified = $uri->withPort(1);
echo "Port 1: ";
var_dump($modified->getPort());

// Test port 65535
$modified = $uri->withPort(65535);
echo "Port 65535: ";
var_dump($modified->getPort());

// Test common non-standard ports
$testPorts = [3000, 8000, 8080, 8443, 9000];
foreach ($testPorts as $port) {
    $modified = $uri->withPort($port);
    echo "Port {$port}: ";
    var_dump($modified->getPort() === $port);
}

// Test that authority includes port when non-standard
$uri = Uri::fromString('http://example.com:8080/path');
echo "Authority with port: ";
var_dump($uri->getAuthority());

// Test that authority excludes standard port
$uri = Uri::fromString('http://example.com:80/path');
echo "Authority without standard port: ";
var_dump($uri->getAuthority());

echo "Port edge cases tests passed\n";
?>
--EXPECT--
Port removed: NULL
Port 1: int(1)
Port 65535: int(65535)
Port 3000: bool(true)
Port 8000: bool(true)
Port 8080: bool(true)
Port 8443: bool(true)
Port 9000: bool(true)
Authority with port: string(16) "example.com:8080"
Authority without standard port: string(11) "example.com"
Port edge cases tests passed
