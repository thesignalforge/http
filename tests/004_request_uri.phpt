--TEST--
signalforge_http: Request PSR-7 RequestInterface getUri() and withUri()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up request environment
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test?foo=bar',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Create request using capture() for setup only
$request = Request::capture();

// ASSERT: RequestInterface - getUri()
var_dump($request->getUri() === '/test?foo=bar');

// ACT: Test withUri() immutability
$uriRequest = $request->withUri('https://example.com/path?query=value');

// ASSERT: Original unchanged, new has different URI
var_dump($uriRequest->getUri() === 'https://example.com/path?query=value');
var_dump($request->getUri() === '/test?foo=bar');
var_dump($request !== $uriRequest);

// ACT: Test withUri() with relative path
$relativeRequest = $request->withUri('/api/users');

// ASSERT: Relative URI set correctly
var_dump($relativeRequest->getUri() === '/api/users');
var_dump($request->getUri() === '/test?foo=bar');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

