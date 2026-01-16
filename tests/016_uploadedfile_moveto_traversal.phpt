--TEST--
signalforge_http: UploadedFile moveTo() rejects directory traversal
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

// ACT & ASSERT: Test /../ traversal rejection
echo "Test /../ traversal: ";
try {
    $uploadedFile->moveTo($testDir . '/../../../etc/passwd');
    echo "FAIL - should have thrown\n";
} catch (InvalidArgumentException $e) {
    var_dump(strpos($e->getMessage(), 'traversal') !== false);
}

// Need new request/file for next test
unset($request, $files, $uploadedFile);
gc_collect_cycles();
file_put_contents($tmpFile, 'test');

$request = Request::capture();
$files = $request->getUploadedFiles();
$uploadedFile = $files['upload'];

// ACT & ASSERT: Test trailing /.. rejection
echo "Test trailing /.. : ";
try {
    $uploadedFile->moveTo($testDir . '/..');
    echo "FAIL - should have thrown\n";
} catch (InvalidArgumentException $e) {
    var_dump(strpos($e->getMessage(), 'traversal') !== false);
}

// Cleanup
@unlink($tmpFile);
@rmdir($testDir);

echo "Traversal rejection tests passed\n";
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
Test /../ traversal: bool(true)
Test trailing /.. : bool(true)
Traversal rejection tests passed
