--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface attribute immutability across all operations
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

// ACT: Get all attributes
$allAttributes = $requestWithAttrs->getAttributes();
var_dump(isset($allAttributes['route']));
var_dump(isset($allAttributes['user_id']));
var_dump(count($allAttributes) === 2);

// ASSERT: Original still empty
var_dump($request->getAttributes() === []);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
