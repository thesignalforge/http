--TEST--
Request::withUri() accepts Uri object and string
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Uri;

$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/original',
    'HTTP_HOST' => 'example.com',
];

$request = Request::capture();

// Test with string (backward compatibility)
$new1 = $request->withUri('https://other.com/new-path');
$uri1 = $new1->getUri();
var_dump($uri1->getHost());
var_dump($uri1->getPath());

echo "---\n";

// Test with Uri object
$uri = Uri::fromString('https://api.example.com:8443/v2/resource?key=value');
$new2 = $request->withUri($uri);
$uri2 = $new2->getUri();
var_dump($uri2->getHost());
var_dump($uri2->getPort());
var_dump($uri2->getPath());
var_dump($uri2->getQuery());

echo "---\n";

// Original request unchanged
$originalUri = $request->getUri();
var_dump($originalUri->getHost());
var_dump($originalUri->getPath());

?>
--EXPECT--
string(9) "other.com"
string(9) "/new-path"
---
string(15) "api.example.com"
int(8443)
string(12) "/v2/resource"
string(9) "key=value"
---
string(11) "example.com"
string(9) "/original"
