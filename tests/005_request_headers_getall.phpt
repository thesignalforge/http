--TEST--
signalforge_http: Request PSR-7 MessageInterface getHeaders() returns lowercase keys per RFC
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

// ASSERT: getHeaders() - returns lowercase keys per RFC
$headers = $request->getHeaders();
var_dump(isset($headers['content-type']));
var_dump(isset($headers['x-custom']));
var_dump(isset($headers['x-multi']));
var_dump(!isset($headers['Content-Type'])); // Should be lowercase
var_dump(is_array($headers));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
