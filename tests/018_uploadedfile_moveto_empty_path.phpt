--TEST--
signalforge_http: UploadedFile moveTo() rejects empty path
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

// ACT & ASSERT: Empty path should be rejected
echo "Empty path rejected: ";
try {
    $uploadedFile->moveTo('');
    echo "FAIL - should have thrown\n";
} catch (InvalidArgumentException $e) {
    var_dump(strpos($e->getMessage(), 'empty') !== false);
}

// Cleanup
@unlink($tmpFile);
@rmdir($testDir);

echo "Empty path rejection test passed\n";
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
Empty path rejected: bool(true)
Empty path rejection test passed
