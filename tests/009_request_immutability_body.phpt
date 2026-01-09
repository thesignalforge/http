--TEST--
signalforge_http: Request PSR-7 immutability withBody()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Request, Stream};

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

// ACT & ASSERT: withBody immutability
$body = Stream::fromString('new body');
$new5 = $request->withBody($body);
var_dump($request !== $new5);
var_dump($request->getBody()->getSize() === 0);
var_dump($new5->getBody()->getContents() === 'new body');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
