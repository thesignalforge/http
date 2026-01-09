--TEST--
signalforge_http: Request PSR-7 immutability withMethod()
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

// ACT & ASSERT: withMethod immutability
$new7 = $request->withMethod('POST');
var_dump($request !== $new7);
var_dump($request->getMethod() === 'GET');
var_dump($new7->getMethod() === 'POST');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
