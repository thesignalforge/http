--TEST--
signalforge_http: Request PSR-7 MessageInterface withoutHeader() removes header
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

// ACT: Remove header
$removedRequest = $request->withoutHeader('Content-Type');

// ASSERT: Header removed from new, original unchanged
var_dump(!$removedRequest->hasHeader('Content-Type'));
var_dump($request->hasHeader('Content-Type'));
var_dump($request !== $removedRequest);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
