--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface withAttribute() adds and replaces attributes
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

// ACT: Add attribute
$request1 = $request->withAttribute('route', 'home');
$request2 = $request1->withAttribute('user_id', 123);
$request3 = $request2->withAttribute('nested', ['key' => 'value']);

// ASSERT: Attributes added
var_dump($request3->getAttribute('route') === 'home');
var_dump($request3->getAttribute('user_id') === 123);
var_dump($request3->getAttribute('nested') === ['key' => 'value']);

// ASSERT: Original unchanged
var_dump($request->getAttributes() === []);
var_dump($request !== $request1);
var_dump($request1 !== $request2);
var_dump($request2 !== $request3);

// ACT: Replace attribute
$request4 = $request3->withAttribute('user_id', 456);

// ASSERT: Attribute replaced
var_dump($request4->getAttribute('user_id') === 456);
var_dump($request3->getAttribute('user_id') === 123); // Original unchanged
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
