--TEST--
signalforge_http: Stream PSR-7 StreamInterface write edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ACT: Test basic write functionality
$resource = fopen('php://memory', 'r+');
$writableStream = Stream::fromResource($resource);
$bytesWritten = $writableStream->write('test content');
var_dump($bytesWritten === 12);
$writableStream->rewind();
var_dump($writableStream->getContents() === 'test content');
?>
--EXPECT--
bool(true)
bool(true)
