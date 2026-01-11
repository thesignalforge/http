--TEST--
Uri::fromString() - basic URI parsing
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// Test basic HTTP URL
$uri = Uri::fromString('http://example.com/path');
var_dump($uri->getScheme());
var_dump($uri->getHost());
var_dump($uri->getPath());
var_dump($uri->getPort());

echo "---\n";

// Test HTTPS with port
$uri = Uri::fromString('https://example.com:8443/api/v1');
var_dump($uri->getScheme());
var_dump($uri->getHost());
var_dump($uri->getPort());
var_dump($uri->getPath());

echo "---\n";

// Test with query and fragment
$uri = Uri::fromString('http://example.com/search?q=test#results');
var_dump($uri->getPath());
var_dump($uri->getQuery());
var_dump($uri->getFragment());

?>
--EXPECT--
string(4) "http"
string(11) "example.com"
string(5) "/path"
NULL
---
string(5) "https"
string(11) "example.com"
int(8443)
string(7) "/api/v1"
---
string(7) "/search"
string(6) "q=test"
string(7) "results"
