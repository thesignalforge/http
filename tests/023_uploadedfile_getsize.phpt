--TEST--
signalforge_http: UploadedFile getSize() returns correct file sizes
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

$testDir = sys_get_temp_dir() . '/signalforge_test_' . getmypid();
@mkdir($testDir, 0755, true);

// Test various file sizes
$testCases = [
    0,          // Empty file
    1,          // Single byte
    100,        // Small file
    1024,       // 1 KB
    1024 * 10,  // 10 KB
    1024 * 100, // 100 KB
];

foreach ($testCases as $size) {
    $tmpFile = $testDir . "/size_test.tmp";
    $content = str_repeat('x', $size);
    file_put_contents($tmpFile, $content);

    $_FILES = [
        'upload' => [
            'name' => 'test.bin',
            'type' => 'application/octet-stream',
            'tmp_name' => $tmpFile,
            'error' => UPLOAD_ERR_OK,
            'size' => $size,
        ],
    ];

    $request = Request::capture();
    $files = $request->getUploadedFiles();
    $uploadedFile = $files['upload'];

    echo "Size {$size} bytes: ";
    var_dump($uploadedFile->getSize() === $size);

    @unlink($tmpFile);
    unset($request, $files, $uploadedFile);
    gc_collect_cycles();
}

@rmdir($testDir);
echo "getSize tests passed\n";
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
Size 0 bytes: bool(true)
Size 1 bytes: bool(true)
Size 100 bytes: bool(true)
Size 1024 bytes: bool(true)
Size 10240 bytes: bool(true)
Size 102400 bytes: bool(true)
getSize tests passed
