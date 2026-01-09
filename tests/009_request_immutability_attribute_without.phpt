--TEST--
signalforge_http: Request PSR-7 immutability withoutAttribute()
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

// ACT & ASSERT: withoutAttribute immutability
$requestWithAttr = $request->withAttribute('temp', 'value');
$new14 = $requestWithAttr->withoutAttribute('temp');
var_dump($requestWithAttr !== $new14);
var_dump($requestWithAttr->getAttribute('temp') === 'value');
var_dump($new14->getAttribute('temp') === null);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
