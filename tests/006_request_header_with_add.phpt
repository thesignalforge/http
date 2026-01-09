--TEST--
signalforge_http: Request PSR-7 MessageInterface withHeader() adds new header
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

// ARRANGE: Test immutability
$originalHeaders = $request->getHeaders();

// ACT: Create new request with header
$newRequest = $request->withHeader('X-New', 'value');

// ASSERT: Original unchanged, new has header
var_dump(!$request->hasHeader('X-New'));
var_dump($newRequest->hasHeader('X-New'));
var_dump($request !== $newRequest);
var_dump($request->getHeaders() === $originalHeaders);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
