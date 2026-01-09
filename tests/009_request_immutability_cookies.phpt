--TEST--
signalforge_http: Request PSR-7 immutability withCookieParams()
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

// ACT & ASSERT: withCookieParams immutability
$new9 = $request->withCookieParams(['new' => 'cookie']);
var_dump($request !== $new9);
var_dump($request->getCookieParams() === ['cookie' => 'value']);
var_dump($new9->getCookieParams() === ['new' => 'cookie']);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
