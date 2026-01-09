--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface withoutAttribute() with non-existent attribute optimization
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

// ACT: Remove non-existent attribute
$request6 = $request->withoutAttribute('non_existent');

// ASSERT: Returns same instance
var_dump($request === $request6);
?>
--EXPECT--
bool(true)
