--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getAttribute() retrieves attributes with default value
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

// ACT: Get non-existent attribute
var_dump($request->getAttribute('non_existent') === null);

// ACT: Get non-existent attribute with default
var_dump($request->getAttribute('non_existent', 'default') === 'default');

// ACT: Get existing attribute
var_dump($requestWithAttrs->getAttribute('route') === 'home');
var_dump($requestWithAttrs->getAttribute('user_id') === 123);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
