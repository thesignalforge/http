--TEST--
signalforge_http: Request PSR-7 immutability withParsedBody()
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

// ACT & ASSERT: withParsedBody immutability
$new12 = $request->withParsedBody(['parsed' => 'data']);
var_dump($request !== $new12);
var_dump($request->getParsedBody() === null);
var_dump($new12->getParsedBody() === ['parsed' => 'data']);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
