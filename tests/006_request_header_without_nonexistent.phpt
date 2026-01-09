--TEST--
signalforge_http: Request PSR-7 MessageInterface withoutHeader() with non-existent header optimization
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

// ACT: Remove non-existent header
$sameRequest = $request->withoutHeader('X-Non-Existent');

// ASSERT: Returns same instance (optimization)
var_dump($request === $sameRequest);
?>
--EXPECT--
bool(true)
