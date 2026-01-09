--TEST--
signalforge_http: Request PSR-7 immutability withHeader()
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

// ACT & ASSERT: withHeader immutability
$new2 = $request->withHeader('X-New', 'value');
var_dump($request !== $new2);
var_dump(!$request->hasHeader('X-New'));
var_dump($new2->hasHeader('X-New'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
