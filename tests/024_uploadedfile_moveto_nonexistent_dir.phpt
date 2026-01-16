--TEST--
signalforge_http: UploadedFile moveTo() rejects non-existent directory
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
file_put_contents($tmpFile, 'test');

$_FILES = [
    'upload' => [
        'name' => 'test.txt',
        'type' => 'text/plain',
        'tmp_name' => $tmpFile,
        'error' => UPLOAD_ERR_OK,
        'size' => 4,
    ],
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['upload'];

// ACT & ASSERT: Non-existent directory should throw RuntimeException
echo "Non-existent directory rejected: ";
try {
    $uploadedFile->moveTo('/nonexistent_directory_12345/file.txt');
    echo "FAIL - should have thrown\n";
} catch (RuntimeException $e) {
    var_dump(strpos($e->getMessage(), 'not exist') !== false ||
             strpos($e->getMessage(), 'not accessible') !== false);
}

// Cleanup
@unlink($tmpFile);
@rmdir($testDir);

echo "Non-existent directory test passed\n";
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
Non-existent directory rejected: bool(true)
Non-existent directory test passed
