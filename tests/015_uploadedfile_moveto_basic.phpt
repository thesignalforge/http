--TEST--
signalforge_http: UploadedFile moveTo() basic functionality
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
$testDir = sys_get_temp_dir() . '/signalforge_test_' . getmypid();
@mkdir($testDir, 0755, true);
$tmpFile = $testDir . '/source.tmp';
$targetFile = $testDir . '/target.txt';
$testContent = 'Test file content for moveTo';
file_put_contents($tmpFile, $testContent);

$_FILES = [
    'upload' => [
        'name' => 'document.txt',
        'type' => 'text/plain',
        'tmp_name' => $tmpFile,
        'error' => UPLOAD_ERR_OK,
        'size' => strlen($testContent),
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['upload'];

// ACT: Move file to target location
$uploadedFile->moveTo($targetFile);

// ASSERT: Target file exists with correct content
echo "Target exists: ";
var_dump(file_exists($targetFile));

echo "Content preserved: ";
var_dump(file_get_contents($targetFile) === $testContent);

echo "Source removed: ";
var_dump(!file_exists($tmpFile));

// Cleanup
@unlink($targetFile);
@rmdir($testDir);

echo "moveTo basic test passed\n";
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
Target exists: bool(true)
Content preserved: bool(true)
Source removed: bool(true)
moveTo basic test passed
