--TEST--
signalforge_http: UploadedFile getClientMediaType() returns correct MIME type
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Setup request environment
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/upload'];
$_GET = [];
$_POST = [];
$_COOKIE = [];

// ARRANGE: Create test file
$testPath = __DIR__ . '/media_type_test.tmp';
file_put_contents($testPath, 'test content');

// ACT & ASSERT: Test text/plain media type
$_FILES = [
    'text_file' => [
        'name' => 'document.txt',
        'type' => 'text/plain',
        'tmp_name' => $testPath,
        'error' => UPLOAD_ERR_OK,
        'size' => 12,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['text_file'];

echo "Text file: ";
var_dump($uploadedFile->getClientMediaType() === 'text/plain');

// Cleanup
unset($request, $files, $uploadedFile);
gc_collect_cycles();

// ACT & ASSERT: Test application/json media type
$_FILES = [
    'json_file' => [
        'name' => 'data.json',
        'type' => 'application/json',
        'tmp_name' => $testPath,
        'error' => UPLOAD_ERR_OK,
        'size' => 12,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['json_file'];

echo "JSON file: ";
var_dump($uploadedFile->getClientMediaType() === 'application/json');

// Cleanup
unset($request, $files, $uploadedFile);
gc_collect_cycles();

// ACT & ASSERT: Test image/png media type
$_FILES = [
    'image_file' => [
        'name' => 'photo.png',
        'type' => 'image/png',
        'tmp_name' => $testPath,
        'error' => UPLOAD_ERR_OK,
        'size' => 12,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['image_file'];

echo "Image file: ";
var_dump($uploadedFile->getClientMediaType() === 'image/png');

// Cleanup
unset($request, $files, $uploadedFile);
gc_collect_cycles();

// ACT & ASSERT: Test empty/null media type
$_FILES = [
    'no_type_file' => [
        'name' => 'unknown',
        'type' => '',
        'tmp_name' => $testPath,
        'error' => UPLOAD_ERR_OK,
        'size' => 12,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['no_type_file'];

echo "Empty type: ";
$mediaType = $uploadedFile->getClientMediaType();
var_dump($mediaType === '' || $mediaType === null);

// Final cleanup
unlink($testPath);
unset($request, $files, $uploadedFile);
gc_collect_cycles();

echo "All media type tests completed\n";
?>
--CLEAN--
<?php
$_SERVER = [];
$_GET = [];
$_POST = [];
$_FILES = [];
$_COOKIE = [];
$_REQUEST = [];
clearstatcache();
gc_collect_cycles();
?>
--EXPECT--
Text file: bool(true)
JSON file: bool(true)
Image file: bool(true)
Empty type: bool(true)
All media type tests completed
