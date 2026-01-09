--TEST--
signalforge_http: Stream PSR-7 StreamInterface seek() with SEEK_END and rewind()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create stream
$stream = Stream::fromString('0123456789');

// ACT: Seek from end
$stream->seek(-3, SEEK_END);

// ASSERT: Position updated from end
var_dump($stream->tell() === 7);
var_dump($stream->read(1) === '7');

// ACT: Rewind to start
$stream->rewind();

// ASSERT: Position at start
var_dump($stream->tell() === 0);
var_dump($stream->read(1) === '0');

// ACT: Seek to end
$stream->seek(0, SEEK_END);

// ASSERT: At end of stream
var_dump($stream->eof());
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
