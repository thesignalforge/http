--TEST--
signalforge_http: Request PSR-7 immutability withAddedHeader()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up base request
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
    'HTTP_X_ORIGINAL' => 'value',
];
$_GET = ['param' => 'value'];
$_POST = [];
$_COOKIE = ['cookie' => 'value'];
$_FILES = [];

$request = Request::capture();

// ACT & ASSERT: withAddedHeader immutability
$new3 = $request->withAddedHeader('X-Original', 'added');
var_dump($request !== $new3);
var_dump($request->getHeader('X-Original') === ['value']);
var_dump($new3->getHeader('X-Original') === ['value', 'added']);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
