--TEST--
signalforge_http: Request PSR-7 MessageInterface getHeader() returns array
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
    'HTTP_X_CUSTOM' => 'value1,value2',
    'HTTP_X_MULTI' => 'first',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: getHeader() - returns array
var_dump($request->getHeader('Content-Type') === ['application/json']);
var_dump($request->getHeader('X-Custom') === ['value1,value2']);
var_dump($request->getHeader('X-Non-Existent') === []);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
