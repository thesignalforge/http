--TEST--
signalforge_http: Request PSR-7 immutability withProtocolVersion()
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

// ACT & ASSERT: withProtocolVersion immutability
$new1 = $request->withProtocolVersion('1.0');
var_dump($request !== $new1);
var_dump($request->getProtocolVersion() === '1.1');
var_dump($new1->getProtocolVersion() === '1.0');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
