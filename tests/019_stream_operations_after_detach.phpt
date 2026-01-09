--TEST--
signalforge_http: Stream PSR-7 StreamInterface operations after detach()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ACT: Create and detach a stream
$resource = fopen('php://memory', 'r+');
fwrite($resource, 'test content');
rewind($resource);
$stream = Stream::fromResource($resource);
$detached = $stream->detach();

// ACT: Test read() after detach
try {
    $stream->read(1);
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test write() after detach
try {
    $stream->write('test');
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test seek() after detach
try {
    $stream->seek(0);
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test tell() after detach
try {
    $stream->tell();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test rewind() after detach
try {
    $stream->rewind();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test getContents() after detach
try {
    $stream->getContents();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test getSize() after detach
$size = $stream->getSize();
var_dump($size === null);

// ACT: Test getMetadata() after detach
$metadata = $stream->getMetadata();
var_dump($metadata === null);

// ACT: Test getMetadata($key) after detach
$specificMetadata = $stream->getMetadata('mode');
var_dump($specificMetadata === null);

// ACT: Test isReadable() after detach
var_dump(!$stream->isReadable());

// ACT: Test isWritable() after detach
var_dump(!$stream->isWritable());

// ACT: Test isSeekable() after detach
var_dump(!$stream->isSeekable());

// ACT: Test eof() after detach
try {
    $stream->eof();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test multiple detach() calls
$resource2 = fopen('php://memory', 'r+');
$stream2 = Stream::fromResource($resource2);
$detached2 = $stream2->detach();
$detachedAgain = $stream2->detach(); // Should return null
var_dump($detachedAgain === null);

// ACT: Test detached resource is still usable
fseek($detached, 0);
$content = fread($detached, 1024);
var_dump($content === 'test content');
fclose($detached);
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
bool(true)
