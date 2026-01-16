--TEST--
signalforge_http: UploadedFile getClientFilename() returns correct values
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

// Test various filename scenarios
$testCases = [
    ['name' => 'simple.txt', 'expected' => 'simple.txt'],
    ['name' => 'file with spaces.pdf', 'expected' => 'file with spaces.pdf'],
    ['name' => 'UPPERCASE.TXT', 'expected' => 'UPPERCASE.TXT'],
    ['name' => 'unicode-日本語.txt', 'expected' => 'unicode-日本語.txt'],
    ['name' => 'dots.in.name.txt', 'expected' => 'dots.in.name.txt'],
    ['name' => '.hidden', 'expected' => '.hidden'],
    ['name' => '', 'expected' => null], // Empty string should return null or empty
];

foreach ($testCases as $i => $testCase) {
    $tmpFile = $testDir . "/test_{$i}.tmp";
    file_put_contents($tmpFile, 'test');

    $_FILES = [
        'upload' => [
            'name' => $testCase['name'],
            'type' => 'text/plain',
            'tmp_name' => $tmpFile,
            'error' => UPLOAD_ERR_OK,
            'size' => 4,
        ],
    ];

    $request = Request::capture();
    $files = $request->getUploadedFiles();
    $uploadedFile = $files['upload'];

    $filename = $uploadedFile->getClientFilename();

    echo "Test '{$testCase['name']}': ";
    if ($testCase['expected'] === null) {
        var_dump($filename === null || $filename === '');
    } else {
        var_dump($filename === $testCase['expected']);
    }

    @unlink($tmpFile);
    unset($request, $files, $uploadedFile);
    gc_collect_cycles();
}

@rmdir($testDir);
echo "getClientFilename tests passed\n";
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
Test 'simple.txt': bool(true)
Test 'file with spaces.pdf': bool(true)
Test 'UPPERCASE.TXT': bool(true)
Test 'unicode-日本語.txt': bool(true)
Test 'dots.in.name.txt': bool(true)
Test '.hidden': bool(true)
Test '': bool(true)
getClientFilename tests passed
