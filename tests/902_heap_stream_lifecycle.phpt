--TEST--
Stream object lifecycle - memory and resource leak prevention
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Test Stream object memory and resource management

echo "=== Test 1: Multiple Stream creation from string ===\n";
for ($i = 0; $i < 20; $i++) {
    $stream = Stream::fromString("Test content {$i}");
    $content = (string)$stream;

    // ASSERT: Stream should work correctly
    var_dump(strlen($content) > 0);
    // Stream destroyed at end of loop - tests cleanup
}
echo "String stream lifecycle passed\n";

echo "\n=== Test 2: Stream operations after detach ===\n";
// Create a writable stream using fromFile with write mode
$tmpfile = tempnam(sys_get_temp_dir(), 'stream_detach_');
$stream = Stream::fromFile($tmpfile, 'w+');
$stream->write("Before detach");
$resource = $stream->detach();

// ASSERT: Stream should be detached
var_dump($resource !== null);
var_dump($stream->isReadable() === false);
var_dump($stream->isWritable() === false);

// Cleanup the detached resource
if (is_resource($resource)) {
    fclose($resource);
}
@unlink($tmpfile);
echo "Detach lifecycle passed\n";

echo "\n=== Test 3: Stream read/seek/rewind cycle ===\n";
$stream = Stream::fromString("0123456789");

for ($i = 0; $i < 5; $i++) {
    $stream->seek(0);
    $data = $stream->read(10);
    // ASSERT: Should read correctly
    var_dump($data === "0123456789");
}
echo "Read/seek cycle passed\n";

echo "\n=== Test 4: Large content read (memory allocation test) ===\n";
$large_content = str_repeat("A", 1024 * 100); // 100KB
$stream = Stream::fromString($large_content);
$read_content = $stream->getContents();

// ASSERT: Content should match
var_dump(strlen($read_content) === strlen($large_content));
var_dump($read_content === $large_content);
echo "Large content passed\n";

echo "\n=== Test 5: Multiple streams with file resources ===\n";
// Create temporary files
$files = [];
for ($i = 0; $i < 10; $i++) {
    $tmpfile = tempnam(sys_get_temp_dir(), 'stream_test_');
    file_put_contents($tmpfile, "File content {$i}");
    $files[] = $tmpfile;

    $stream = Stream::fromFile($tmpfile, 'r');
    $content = (string)$stream;

    // ASSERT: File content should be correct
    var_dump($content === "File content {$i}");
    // Stream destroyed, but file remains for cleanup
}

// Cleanup temporary files
foreach ($files as $file) {
    @unlink($file);
}
echo "File stream lifecycle passed\n";

echo "\n=== Test 6: Stream close and operations ===\n";
$stream = Stream::fromString("Test");
$stream->close();

// ASSERT: Stream should be closed
var_dump($stream->isReadable() === false);
var_dump($stream->isWritable() === false);
echo "Close lifecycle passed\n";

echo "\n=== Test 7: Rapid stream creation/destruction ===\n";
for ($i = 0; $i < 50; $i++) {
    $s = Stream::fromString("Iteration {$i}");
    $content = (string)$s;
    // Stream destroyed at end of loop
}
echo "Rapid lifecycle passed\n";

echo "\nAll Stream heap tests passed\n";
?>
--EXPECT--
=== Test 1: Multiple Stream creation from string ===
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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
String stream lifecycle passed

=== Test 2: Stream operations after detach ===
bool(true)
bool(true)
bool(true)
Detach lifecycle passed

=== Test 3: Stream read/seek/rewind cycle ===
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
Read/seek cycle passed

=== Test 4: Large content read (memory allocation test) ===
bool(true)
bool(true)
Large content passed

=== Test 5: Multiple streams with file resources ===
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
File stream lifecycle passed

=== Test 6: Stream close and operations ===
bool(true)
bool(true)
Close lifecycle passed

=== Test 7: Rapid stream creation/destruction ===
Rapid lifecycle passed

All Stream heap tests passed
