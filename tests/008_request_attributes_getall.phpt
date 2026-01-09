--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getAttributes() returns default empty array
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up basic request
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: Default empty attributes
var_dump($request->getAttributes() === []);
?>
--EXPECT--
bool(true)
