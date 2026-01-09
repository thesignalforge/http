--TEST--
signalforge_http: Request PSR-7 MessageInterface getHeaderLine() returns comma-separated string
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

// ASSERT: getHeaderLine() - returns comma-separated string
var_dump($request->getHeaderLine('Content-Type') === 'application/json');
var_dump($request->getHeaderLine('X-Custom') === 'value1,value2');
var_dump($request->getHeaderLine('X-Non-Existent') === '');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
