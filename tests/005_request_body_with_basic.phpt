--TEST--
signalforge_http: Request PSR-7 MessageInterface withBody() basic replacement and immutability
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

// ARRANGE: Create stream body
$streamBody = Stream::fromString('test body content');

// ACT: Create request with body
$newRequest = $request->withBody($streamBody);

// ASSERT: Body replaced, original unchanged
var_dump($newRequest->getBody()->getContents() === 'test body content');
var_dump($request->getBody()->getSize() === 0); // Original unchanged
var_dump($request !== $newRequest);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
