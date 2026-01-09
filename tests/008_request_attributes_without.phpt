--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface withoutAttribute() removes attribute
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

// ACT: Add some attributes
$requestWithAttrs = $request->withAttribute('route', 'home')->withAttribute('user_id', 123);

// ACT: Remove attribute
$request5 = $requestWithAttrs->withoutAttribute('user_id');

// ASSERT: Attribute removed
var_dump($request5->getAttribute('user_id') === null);
var_dump($requestWithAttrs->getAttribute('user_id') === 123); // Original unchanged
var_dump($requestWithAttrs !== $request5);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
