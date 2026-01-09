--TEST--
signalforge_http: Stream PSR-7 StreamInterface eof() behavior
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create readable stream
$stream = Stream::fromString('Hello World');

// ACT: Read from stream
$data1 = $stream->read(5);

// ACT: Read remaining data
$data2 = $stream->getContents();

// ASSERT: At end of stream
var_dump($stream->eof());
?>
--EXPECT--
bool(true)
