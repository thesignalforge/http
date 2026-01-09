--TEST--
signalforge_http: Stream PSR-7 StreamInterface operations after close()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ACT: Create and close a stream
$stream = Stream::fromString('test content');
$stream->close();

// ACT: Test read() after close
try {
    $stream->read(1);
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test write() after close
try {
    $stream->write('test');
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test seek() after close
try {
    $stream->seek(0);
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test tell() after close
try {
    $stream->tell();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test rewind() after close
try {
    $stream->rewind();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test getContents() after close
try {
    $stream->getContents();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test getSize() after close
$size = $stream->getSize();
var_dump($size === null);

// ACT: Test getMetadata() after close
$metadata = $stream->getMetadata();
var_dump($metadata === null);

// ACT: Test getMetadata($key) after close
$specificMetadata = $stream->getMetadata('mode');
var_dump($specificMetadata === null);

// ACT: Test isReadable() after close
var_dump(!$stream->isReadable());

// ACT: Test isWritable() after close
var_dump(!$stream->isWritable());

// ACT: Test isSeekable() after close
var_dump(!$stream->isSeekable());

// ACT: Test eof() after close
try {
    $stream->eof();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test multiple close() calls
$stream2 = Stream::fromString('test');
$stream2->close();
$stream2->close(); // Should not throw
var_dump(true); // If we reach here, multiple closes are OK
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
