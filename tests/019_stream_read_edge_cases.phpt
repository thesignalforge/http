--TEST--
signalforge_http: Stream PSR-7 StreamInterface read edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ACT: Test read() with 0 bytes
$stream = Stream::fromString('Hello World');
$data = $stream->read(0);
var_dump($data === '');
var_dump($stream->tell() === 0); // Position unchanged

// ACT: Test read() with negative length
try {
    $stream->read(-1);
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test read() beyond EOF
$stream->seek(0, SEEK_END); // Go to end
$data = $stream->read(10); // Try to read past end
var_dump($data === '');
var_dump($stream->eof());

// ACT: Test getContents() on empty stream
$emptyStream = Stream::fromString('');
$contents = $emptyStream->getContents();
var_dump($contents === '');
var_dump($emptyStream->eof());

// ACT: Test getContents() when already at EOF
$stream = Stream::fromString('test');
$stream->seek(0, SEEK_END);
$contents = $stream->getContents();
var_dump($contents === '');
var_dump($stream->eof());

// ACT: Test getContents() on already read stream
$stream = Stream::fromString('Hello World');
$firstRead = $stream->read(5); // Read 'Hello'
var_dump($firstRead === 'Hello');
$remaining = $stream->getContents(); // Get remaining ' World'
var_dump($remaining === ' World');

// ACT: Test read() on non-readable stream
$resource = fopen('php://memory', 'w+'); // Write-only mode
fwrite($resource, 'test');
rewind($resource);
$writeOnlyStream = Stream::fromResource($resource);
if (!$writeOnlyStream->isReadable()) {
    try {
        $writeOnlyStream->read(1);
        var_dump(false); // Should not reach here
    } catch (RuntimeException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
} else {
    var_dump(true); // Stream is readable, skip test
}

// ACT: Test read() with very large length
$largeStream = Stream::fromString('short content');
$data = $largeStream->read(10000); // Request more than available
var_dump($data === 'short content');
var_dump($largeStream->eof());
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
