--TEST--
signalforge_http: Request PSR-7 immutability withUri()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Uri;

// ARRANGE: Set up base request
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
    'HTTP_HOST' => 'localhost',
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

// Original URI should still have original path
$originalUri = $request->getUri();
var_dump($originalUri->getPath() === '/test');

// New URI should have new host and path
$newUri = $new8->getUri();
var_dump($newUri->getHost() === 'example.com');
var_dump($newUri->getPath() === '/new');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
