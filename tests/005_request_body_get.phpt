--TEST--
signalforge_http: Request PSR-7 MessageInterface getBody() returns default empty Stream
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Request, Stream};

// ARRANGE: Set up request
$_SERVER = [
    'REQUEST_METHOD' => 'POST',
    'REQUEST_URI' => '/test',
    'HTTP_CONTENT_TYPE' => 'application/json',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ACT: Get body
$body = $request->getBody();

// ASSERT: Default empty body
var_dump($body instanceof Stream);
var_dump($body->getSize() === 0);
var_dump($body->getContents() === '');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
