--TEST--
signalforge_http: Request PSR-7 MessageInterface withBody() with empty stream
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

// ARRANGE: Test with empty stream
$emptyStream = Stream::fromString('');

// ACT: Set empty body
$emptyBodyRequest = $request->withBody($emptyStream);

// ASSERT: Empty body set
var_dump($emptyBodyRequest->getBody()->getSize() === 0);
var_dump($emptyBodyRequest->getBody()->getContents() === '');
?>
--EXPECT--
bool(true)
bool(true)
