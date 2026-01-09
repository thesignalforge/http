--TEST--
signalforge_http: Request PSR-7 immutability withAttribute()
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

// ACT & ASSERT: withAttribute immutability
$new13 = $request->withAttribute('attr', 'value');
var_dump($request !== $new13);
var_dump($request->getAttribute('attr') === null);
var_dump($new13->getAttribute('attr') === 'value');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
