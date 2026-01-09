--TEST--
signalforge_http: Request PSR-7 immutability withQueryParams()
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

// ACT & ASSERT: withQueryParams immutability
$new10 = $request->withQueryParams(['new' => 'param']);
var_dump($request !== $new10);
var_dump($request->getQueryParams() === ['param' => 'value']);
var_dump($new10->getQueryParams() === ['new' => 'param']);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
