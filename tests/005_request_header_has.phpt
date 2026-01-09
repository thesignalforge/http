--TEST--
signalforge_http: Request PSR-7 MessageInterface hasHeader() case-insensitive behavior
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

// ASSERT: hasHeader() - case insensitive
var_dump($request->hasHeader('Content-Type'));
var_dump($request->hasHeader('content-type'));
var_dump($request->hasHeader('CONTENT-TYPE'));
var_dump(!$request->hasHeader('X-Non-Existent'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
