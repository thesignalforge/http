--TEST--
signalforge_http: Stream PSR-7 StreamInterface seek() with SEEK_CUR
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create stream
$stream = Stream::fromString('0123456789');

// ACT: Seek to position 5 first
$stream->seek(5);

// ACT: Seek from current position
$stream->seek(2, SEEK_CUR);

// ASSERT: Position updated from current
var_dump($stream->tell() === 7);
var_dump($stream->read(1) === '7');
?>
--EXPECT--
bool(true)
bool(true)
