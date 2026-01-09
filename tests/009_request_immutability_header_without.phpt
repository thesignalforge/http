--TEST--
signalforge_http: Request PSR-7 immutability withoutHeader()
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

// ACT & ASSERT: withoutHeader immutability
$new4 = $request->withoutHeader('X-Original');
var_dump($request !== $new4);
var_dump($request->hasHeader('X-Original'));
var_dump(!$new4->hasHeader('X-Original'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
