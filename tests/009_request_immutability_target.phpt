--TEST--
signalforge_http: Request PSR-7 immutability withRequestTarget()
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

// ACT & ASSERT: withRequestTarget immutability
$new6 = $request->withRequestTarget('/new-target');
var_dump($request !== $new6);
var_dump($request->getRequestTarget() === '/test');
var_dump($new6->getRequestTarget() === '/new-target');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
