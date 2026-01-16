--TEST--
signalforge_http: UploadedFile getStream() returns valid StreamInterface
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Psr\Http\Message\StreamInterface;

// ARRANGE: Setup request environment
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/upload'];
$_GET = [];
$_POST = [];
$_COOKIE = [];

// ARRANGE: Create test file
$testDir = sys_get_temp_dir() . '/signalforge_test_' . getmypid();
@mkdir($testDir, 0755, true);
$tmpFile = $testDir . '/stream_test.tmp';
$testContent = 'Stream test content with special chars: <>&"\'';
file_put_contents($tmpFile, $testContent);

$_FILES = [
    'upload' => [
        'name' => 'test.txt',
        'type' => 'text/plain',
        'tmp_name' => $tmpFile,
        'error' => UPLOAD_ERR_OK,
        'size' => strlen($testContent),
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['upload'];

// ACT: Get stream
$stream = $uploadedFile->getStream();

// ASSERT: Stream implements PSR-7 interface
echo "Implements StreamInterface: ";
var_dump($stream instanceof StreamInterface);

// ASSERT: Stream is readable
echo "Stream is readable: ";
var_dump($stream->isReadable());

// ASSERT: Can read content
echo "Content matches: ";
$readContent = $stream->getContents();
var_dump($readContent === $testContent);

// ASSERT: Multiple getStream() calls return same cached stream
$stream2 = $uploadedFile->getStream();
echo "Stream cached: ";
var_dump($stream === $stream2);

// Cleanup
@unlink($tmpFile);
@rmdir($testDir);

echo "getStream tests passed\n";
?>
--CLEAN--
<?php
$_SERVER = [];
$_GET = [];
$_POST = [];
$_FILES = [];
$_COOKIE = [];
gc_collect_cycles();
?>
--EXPECT--
Implements StreamInterface: bool(true)
Stream is readable: bool(true)
Content matches: bool(true)
Stream cached: bool(true)
getStream tests passed
