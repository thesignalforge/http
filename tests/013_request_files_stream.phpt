--TEST--
signalforge_http: UploadedFile getStream() method works correctly with valid and invalid files
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// Test 1: Valid file with getStream()
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/upload'];
$_GET = [];
$_POST = [];
$_COOKIE = [];

$tempPath = __DIR__ . '/test_upload.tmp';
file_put_contents($tempPath, 'test content');

$_FILES = [
    'valid_file' => [
        'name' => 'test.txt',
        'type' => 'text/plain',
        'tmp_name' => $tempPath,
        'error' => UPLOAD_ERR_OK,
        'size' => 12,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['valid_file'];

try {
    $stream = $uploadedFile->getStream();
    $content = $stream->getContents();
    var_dump($content === 'test content');
} catch (Exception $e) {
    var_dump('ERROR: ' . $e->getMessage());
}

unset($request, $files, $uploadedFile, $stream);

// Reset superglobals to prevent state pollution between test sections
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/upload'];
$_GET = [];
$_POST = [];
$_COOKIE = [];

// Test 2: Invalid file path should throw RuntimeException
$_FILES = [
    'invalid_file' => [
        'name' => 'invalid.txt',
        'type' => 'text/plain',
        'tmp_name' => '/definitely/this/path/does/not/exist/at/all.txt',
        'error' => UPLOAD_ERR_OK,
        'size' => 100,
    ],
];

$request2 = Request::capture();
$files2 = $request2->getUploadedFiles();
$uploadedFile2 = $files2['invalid_file'];

try {
    $stream2 = $uploadedFile2->getStream();
    var_dump(false); // Should not reach here
} catch (RuntimeException $e) {
    var_dump(strpos($e->getMessage(), 'Unable to open uploaded file for reading') === 0);
} catch (Exception $e) {
    var_dump('Wrong exception type: ' . get_class($e));
}

unset($request2, $files2, $uploadedFile2, $stream2);

// Reset superglobals to prevent state pollution between test sections
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/upload'];
$_GET = [];
$_POST = [];
$_COOKIE = [];

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

$request3 = Request::capture();
$files3 = $request3->getUploadedFiles();
$uploadedFile3 = $files3['error_file'];

var_dump($uploadedFile3->getError() === UPLOAD_ERR_INI_SIZE);

unset($request3, $files3, $uploadedFile3);

// Clean up
unlink($tempPath);
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
