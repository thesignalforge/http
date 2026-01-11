--TEST--
signalforge_http: Request PSR-7 RequestInterface getUri() and withUri()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Uri;

// ARRANGE: Set up request environment
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test?foo=bar',
    'HTTP_HOST' => 'localhost',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Create request using capture() for setup only
$request = Request::capture();

// ASSERT: RequestInterface - getUri() returns Uri object
$uri = $request->getUri();
var_dump($uri instanceof Uri);
var_dump($uri->getPath() === '/test');
var_dump($uri->getQuery() === 'foo=bar');

// ACT: Test withUri() immutability with string
$uriRequest = $request->withUri('https://example.com/path?query=value');
$newUri = $uriRequest->getUri();

// ASSERT: Original unchanged, new has different URI
var_dump($newUri->getHost() === 'example.com');
var_dump($newUri->getPath() === '/path');
var_dump($newUri->getQuery() === 'query=value');
var_dump($request !== $uriRequest);

// ACT: Test withUri() with Uri object
$uri2 = Uri::fromString('https://api.example.com/v1/users');
$apiRequest = $request->withUri($uri2);
$apiUri = $apiRequest->getUri();

// ASSERT: Uri object passed correctly
var_dump($apiUri->getHost() === 'api.example.com');
var_dump($apiUri->getPath() === '/v1/users');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
