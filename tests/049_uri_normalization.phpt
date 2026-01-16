--TEST--
signalforge_http: Uri normalizes components correctly
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// Test scheme normalization (lowercase)
$uri = Uri::fromString('HTTP://EXAMPLE.COM/path');
echo "Scheme normalized: ";
var_dump($uri->getScheme());

// Test host normalization (lowercase)
echo "Host normalized: ";
var_dump($uri->getHost());

// Test non-standard port preservation
$uri = Uri::fromString('http://example.com:8080/');
echo "Non-standard port preserved: ";
var_dump($uri->getPort());

// Test standard ports are null
$uri = Uri::fromString('http://example.com:80/');
echo "HTTP 80 returns null: ";
var_dump($uri->getPort());

$uri = Uri::fromString('https://example.com:443/');
echo "HTTPS 443 returns null: ";
var_dump($uri->getPort());

// Test path normalization (no double slashes)
$uri = Uri::fromString('http://example.com//double//slashes');
echo "Path preserved: ";
var_dump($uri->getPath());

// Test __toString reconstructs properly
$uri = Uri::fromString('https://user:pass@example.com:8443/path?query=value#fragment');
echo "toString reconstruction: ";
var_dump((string)$uri);

echo "Normalization tests passed\n";
?>
--EXPECT--
Scheme normalized: string(4) "http"
Host normalized: string(11) "example.com"
Non-standard port preserved: int(8080)
HTTP 80 returns null: NULL
HTTPS 443 returns null: NULL
Path preserved: string(17) "//double//slashes"
toString reconstruction: string(60) "https://user:pass@example.com:8443/path?query=value#fragment"
Normalization tests passed
