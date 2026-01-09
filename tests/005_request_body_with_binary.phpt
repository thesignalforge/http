--TEST--
signalforge_http: Request PSR-7 MessageInterface withBody() with binary data
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

// ARRANGE: Test with binary data
$binaryData = "\x00\x01\x02\x03\xFF\xFE\xFD";
$binaryStream = Stream::fromString($binaryData);

// ACT: Set binary body
$binaryBodyRequest = $request->withBody($binaryStream);

// ASSERT: Binary data preserved
var_dump($binaryBodyRequest->getBody()->getContents() === $binaryData);
var_dump($binaryBodyRequest->getBody()->getSize() === 7);
?>
--EXPECT--
bool(true)
bool(true)
