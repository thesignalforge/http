--TEST--
signalforge_http: Uri handles empty and missing components correctly
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// Test URI with minimal components
$uri = Uri::fromString('/path/only');
echo "Path only - scheme: ";
var_dump($uri->getScheme());
echo "Path only - host: ";
var_dump($uri->getHost());
echo "Path only - path: ";
var_dump($uri->getPath());

echo "---\n";

// Test URI with empty query
$uri = Uri::fromString('http://example.com/path?');
echo "Empty query: ";
var_dump($uri->getQuery());

// Test URI with empty fragment
$uri = Uri::fromString('http://example.com/path#');
echo "Empty fragment: ";
var_dump($uri->getFragment());

echo "---\n";

// Test scheme-relative URI (// prefix)
$uri = Uri::fromString('//example.com/path');
echo "Scheme-relative - scheme: ";
var_dump($uri->getScheme());
echo "Scheme-relative - host: ";
var_dump($uri->getHost());
echo "Scheme-relative - path: ";
var_dump($uri->getPath());

echo "---\n";

// Test URI with only host
$uri = Uri::fromString('http://example.com');
echo "Host only - path: ";
var_dump($uri->getPath());

echo "Empty/missing components tests passed\n";
?>
--EXPECT--
Path only - scheme: string(0) ""
Path only - host: string(0) ""
Path only - path: string(10) "/path/only"
---
Empty query: string(0) ""
Empty fragment: string(0) ""
---
Scheme-relative - scheme: string(0) ""
Scheme-relative - host: string(11) "example.com"
Scheme-relative - path: string(5) "/path"
---
Host only - path: string(0) ""
Empty/missing components tests passed
