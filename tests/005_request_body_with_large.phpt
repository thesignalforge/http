--TEST--
signalforge_http: Request PSR-7 MessageInterface withBody() with large content
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

// ARRANGE: Test with large body
$largeContent = str_repeat('x', 10000);
$largeStream = Stream::fromString($largeContent);

// ACT: Set large body
$largeBodyRequest = $request->withBody($largeStream);

// ASSERT: Large body set correctly
var_dump($largeBodyRequest->getBody()->getSize() === 10000);
var_dump(strlen($largeBodyRequest->getBody()->getContents()) === 10000);
?>
--EXPECT--
bool(true)
bool(true)
