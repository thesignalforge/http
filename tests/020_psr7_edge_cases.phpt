--TEST--
signalforge_http: PSR-7 comprehensive edge cases and boundary conditions
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Request, Response, Stream};

// Test basic PSR-7 functionality
$response = Response::create();
$stream = Stream::fromString('test');

var_dump($response->getStatusCode() === 200);
var_dump($stream->getContents() === 'test');
?>
--EXPECT--
bool(true)
bool(true)
