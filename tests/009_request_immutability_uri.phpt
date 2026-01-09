--TEST--
signalforge_http: Request PSR-7 immutability withUri()
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

// ACT & ASSERT: withUri immutability
$new8 = $request->withUri('https://example.com/new');
var_dump($request !== $new8);
var_dump($request->getUri() === '/test');
var_dump($new8->getUri() === 'https://example.com/new');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
