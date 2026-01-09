--TEST--
signalforge_http: Request PSR-7 MessageInterface withHeader() replaces existing header
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up request with headers
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
    'HTTP_CONTENT_TYPE' => 'application/json',
    'HTTP_X_MULTI' => 'first',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ACT: Replace existing header
$replacedRequest = $request->withHeader('Content-Type', 'text/html');

// ASSERT: Original unchanged, new has different value
var_dump($request->getHeader('Content-Type') === ['application/json']);
var_dump($replacedRequest->getHeader('Content-Type') === ['text/html']);
var_dump($request !== $replacedRequest);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
