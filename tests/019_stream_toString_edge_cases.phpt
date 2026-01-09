--TEST--
signalforge_http: Stream PSR-7 StreamInterface __toString() edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ACT: Test __toString() on empty stream
$emptyStream = Stream::fromString('');
$content = (string)$emptyStream;
var_dump($content === '');

// ACT: Test __toString() on normal stream
$normalStream = Stream::fromString('Hello World');
$content = (string)$normalStream;
var_dump($content === 'Hello World');

// ACT: Test __toString() on closed stream
$closedStream = Stream::fromString('test content');
$closedStream->close();
$content = (string)$closedStream;
var_dump($content === ''); // Closed streams should return empty string

// ACT: Test __toString() on detached stream
$resource = fopen('php://memory', 'r+');
fwrite($resource, 'detached content');
rewind($resource);
$detachedStream = Stream::fromResource($resource);
$detachedStream->detach();
$content = (string)$detachedStream;
var_dump($content === ''); // Detached streams should return empty string

// ACT: Test __toString() on very large stream
$largeContent = str_repeat('x', 100 * 1024); // 100KB
$largeStream = Stream::fromString($largeContent);
$content = (string)$largeStream;
var_dump(strlen($content) === 100 * 1024);
var_dump($content === $largeContent);

// ACT: Test __toString() with special characters
$specialContent = "Content with special chars: \n\r\t\x00\xff";
$specialStream = Stream::fromString($specialContent);
$content = (string)$specialStream;
var_dump($content === $specialContent);

// ACT: Test __toString() preserves stream position
$positionStream = Stream::fromString('Hello World');
$positionStream->read(5); // Read 'Hello', position now at 5
$content = (string)$positionStream;
var_dump($content === 'Hello World'); // Should return full content regardless of position
var_dump($positionStream->tell() === 5); // Position should be unchanged

// ACT: Test __toString() on resource-based stream
$resource2 = fopen('php://memory', 'r+');
fwrite($resource2, 'resource content');
rewind($resource2);
$resourceStream = Stream::fromResource($resource2);
$content = (string)$resourceStream;
var_dump($content === 'resource content');
fclose($resource2);

// ACT: Test __toString() multiple calls return same content
$multiCallStream = Stream::fromString('consistent content');
$content1 = (string)$multiCallStream;
$content2 = (string)$multiCallStream;
var_dump($content1 === $content2);
var_dump($content1 === 'consistent content');

// ACT: Test __toString() doesn't affect stream operations
$operationStream = Stream::fromString('test content');
$beforeToString = $operationStream->tell();
$content = (string)$operationStream;
$afterToString = $operationStream->tell();
var_dump($beforeToString === $afterToString); // Position unchanged
var_dump($operationStream->read(4) === 'test'); // Can still read
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
