--TEST--
signalforge_http: Stream PSR-7 StreamInterface factory method edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ACT: Test fromString() with empty string
$emptyStream = Stream::fromString('');
var_dump($emptyStream->getSize() === 0);
var_dump($emptyStream->getContents() === '');
var_dump($emptyStream->eof());

// ACT: Test fromString() with very large string
$largeString = str_repeat('x', 1024 * 1024); // 1MB
$largeStream = Stream::fromString($largeString);
var_dump($largeStream->getSize() === 1024 * 1024);
var_dump(strlen($largeStream->getContents()) === 1024 * 1024);

// ACT: Test fromString() with special characters
$specialString = "String with special chars: \n\r\t\x00\xff";
$specialStream = Stream::fromString($specialString);
var_dump($specialStream->getContents() === $specialString);

// ACT: Test fromResource() with invalid resource types
$invalidResources = [
    'string', // String
    123, // Integer
    12.34, // Float
    true, // Boolean
    false, // Boolean
    [], // Array
    new stdClass(), // Object
    null, // Null
];

foreach ($invalidResources as $invalidResource) {
    try {
        Stream::fromResource($invalidResource);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test fromResource() with closed resource
$resource = fopen('php://memory', 'r+');
fclose($resource); // Close it first
try {
    Stream::fromResource($resource);
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test fromResource() with non-stream resource
$tempFile = tmpfile(); // Creates a file resource, not stream
try {
    Stream::fromResource($tempFile);
    var_dump(true); // File resources should work
} catch (Exception $e) {
    var_dump(false); // Should work with file resources
}
fclose($tempFile);

// ACT: Test fromFile() with non-existent file
try {
    Stream::fromFile('/non/existent/file.txt');
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test fromFile() with directory
try {
    Stream::fromFile('/tmp'); // Directory
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test fromFile() with non-existent file
$nonExistentFile = '/tmp/definitely_does_not_exist_' . uniqid() . '.test';

// Try to create stream from non-existent file
try {
    Stream::fromFile($nonExistentFile, 'r');
    var_dump(true); // Should not reach here
} catch (Exception $e) {
    var_dump(false); // Exception correctly thrown for missing file
}

// ACT: Test fromFile() with different modes
$filename2 = tempnam(sys_get_temp_dir(), 'stream_test');
file_put_contents($filename2, 'test content');

$fileStream = Stream::fromFile($filename2, 'r');
var_dump($fileStream->getContents() === 'test content');
var_dump(!$fileStream->isWritable()); // Read-only mode

// Clean up
@unlink($filename2);
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
bool(false)
bool(true)
bool(true)
bool(false)
bool(false)
bool(true)
bool(false)
