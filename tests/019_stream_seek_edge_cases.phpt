--TEST--
signalforge_http: Stream PSR-7 StreamInterface seek edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ACT: Test basic seek functionality
$stream = Stream::fromString('Hello World');
$stream->seek(6);
$char = $stream->read(1);
var_dump($char === 'W');
var_dump($stream->tell() === 7);
?>
--EXPECT--
bool(true)
bool(true)
