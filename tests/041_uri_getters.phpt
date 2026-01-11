--TEST--
Uri getters - all component accessors
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// Full URI with all components
$uri = Uri::fromString('https://user:pass@example.com:8443/path/to/resource?key=value&foo=bar#section');

echo "Scheme: ";
var_dump($uri->getScheme());

echo "UserInfo: ";
var_dump($uri->getUserInfo());

echo "Host: ";
var_dump($uri->getHost());

echo "Port: ";
var_dump($uri->getPort());

echo "Path: ";
var_dump($uri->getPath());

echo "Query: ";
var_dump($uri->getQuery());

echo "Fragment: ";
var_dump($uri->getFragment());

echo "Authority: ";
var_dump($uri->getAuthority());

echo "---\n";

// Standard port should return null
$uri = Uri::fromString('http://example.com:80/');
echo "HTTP port 80: ";
var_dump($uri->getPort());

$uri = Uri::fromString('https://example.com:443/');
echo "HTTPS port 443: ";
var_dump($uri->getPort());

?>
--EXPECT--
Scheme: string(5) "https"
UserInfo: string(9) "user:pass"
Host: string(11) "example.com"
Port: int(8443)
Path: string(17) "/path/to/resource"
Query: string(17) "key=value&foo=bar"
Fragment: string(7) "section"
Authority: string(26) "user:pass@example.com:8443"
---
HTTP port 80: NULL
HTTPS port 443: NULL
