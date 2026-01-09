--TEST--
signalforge_http: UploadedFile basic functionality with various file types
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// Test 1: Basic file functionality
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/upload'];
$_GET = [];
$_POST = [];
$_COOKIE = [];

$testPath = __DIR__ . '/test_basic.tmp';
file_put_contents($testPath, 'basic content');

$_FILES = [
    'basic_file' => [
        'name' => 'test.txt',
        'type' => 'text/plain',
        'tmp_name' => $testPath,
        'error' => UPLOAD_ERR_OK,
        'size' => 13,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['basic_file'];

try {
    $stream = $uploadedFile->getStream();
    $content = $stream->getContents();
    var_dump($content === 'basic content');
} catch (Exception $e) {
    var_dump('Basic file failed: ' . $e->getMessage());
}

unlink($testPath);

// Cleanup after test 1
unset($request, $files, $uploadedFile, $stream);
gc_collect_cycles();

// Test 2: Empty file
$emptyPath = __DIR__ . '/empty.tmp';
file_put_contents($emptyPath, '');

$_FILES = [
    'empty_file' => [
        'name' => 'empty.txt',
        'type' => 'text/plain',
        'tmp_name' => $emptyPath,
        'error' => UPLOAD_ERR_OK,
        'size' => 0,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['empty_file'];

try {
    $stream = $uploadedFile->getStream();
    $content = $stream->getContents();
    var_dump($content === '');
} catch (Exception $e) {
    var_dump('Empty file failed: ' . $e->getMessage());
}

unlink($emptyPath);

// Cleanup after test 2
unset($request, $files, $uploadedFile, $stream);
gc_collect_cycles();

// Test 3: File with upload error should have correct error code
$_FILES = [
    'error_file' => [
        'name' => 'error.txt',
        'type' => 'text/plain',
        'tmp_name' => '/tmp/error',
        'error' => UPLOAD_ERR_INI_SIZE,
        'size' => 0,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['error_file'];

var_dump($uploadedFile->getError() === UPLOAD_ERR_INI_SIZE);

// Cleanup after test 3
unset($request, $files, $uploadedFile, $stream);
gc_collect_cycles();

// Test 4: Move to valid location
$sourcePath = __DIR__ . '/source.tmp';
$targetPath = __DIR__ . '/target.tmp';
file_put_contents($sourcePath, 'source content');

$_FILES = [
    'move_file' => [
        'name' => 'move.txt',
        'type' => 'text/plain',
        'tmp_name' => $sourcePath,
        'error' => UPLOAD_ERR_OK,
        'size' => 14,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['move_file'];

try {
    $uploadedFile->moveTo($targetPath);
    $content = file_get_contents($targetPath);
    var_dump($content === 'source content');
    // File was moved, only cleanup target
    unlink($targetPath);
} catch (Exception $e) {
    var_dump('Move failed: ' . $e->getMessage());
    // Move failed, cleanup source
    unlink($sourcePath);
}

// Cleanup after test 4
unset($request, $files, $uploadedFile, $sourcePath, $targetPath, $content);
gc_collect_cycles();

// Test 5: File with special characters in name
$_FILES = [
    'special_file' => [
        'name' => 'file with spaces & special chars.tmp',
        'type' => 'text/plain',
        'tmp_name' => __DIR__ . '/special.tmp',
        'error' => UPLOAD_ERR_OK,
        'size' => 10,
    ],
];

file_put_contents(__DIR__ . '/special.tmp', 'special');

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['special_file'];

try {
    $stream = $uploadedFile->getStream();
    $content = $stream->getContents();
    var_dump($content === 'special');
} catch (Exception $e) {
    var_dump('Special file failed: ' . $e->getMessage());
}

unlink(__DIR__ . '/special.tmp');

echo "All tests completed\n";

// Explicit cleanup to prevent state pollution between tests
unset($request, $files, $uploadedFile, $stream, $sourcePath, $targetPath, $testPath, $emptyPath, $newFiles);
gc_collect_cycles();
?>
--CLEAN--
<?php
// Reset superglobals to prevent state pollution between tests
$_SERVER = [];
$_GET = [];
$_POST = [];
$_FILES = [];
$_COOKIE = [];
$_REQUEST = [];

// Clear stat cache
clearstatcache();

// Force garbage collection to clean up cached objects
gc_collect_cycles();
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
All tests completed
