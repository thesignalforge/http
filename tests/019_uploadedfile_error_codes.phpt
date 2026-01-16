--TEST--
signalforge_http: UploadedFile handles upload error codes correctly
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

// Test each PHP upload error constant
$errorCodes = [
    UPLOAD_ERR_OK => 'UPLOAD_ERR_OK',
    UPLOAD_ERR_INI_SIZE => 'UPLOAD_ERR_INI_SIZE',
    UPLOAD_ERR_FORM_SIZE => 'UPLOAD_ERR_FORM_SIZE',
    UPLOAD_ERR_PARTIAL => 'UPLOAD_ERR_PARTIAL',
    UPLOAD_ERR_NO_FILE => 'UPLOAD_ERR_NO_FILE',
    UPLOAD_ERR_NO_TMP_DIR => 'UPLOAD_ERR_NO_TMP_DIR',
    UPLOAD_ERR_CANT_WRITE => 'UPLOAD_ERR_CANT_WRITE',
    UPLOAD_ERR_EXTENSION => 'UPLOAD_ERR_EXTENSION',
];

$testDir = sys_get_temp_dir() . '/signalforge_test_' . getmypid();
@mkdir($testDir, 0755, true);

foreach ($errorCodes as $code => $name) {
    $tmpFile = $testDir . '/error_test.tmp';
    file_put_contents($tmpFile, 'test');

    $_FILES = [
        'upload' => [
            'name' => 'test.txt',
            'type' => 'text/plain',
            'tmp_name' => $tmpFile,
            'error' => $code,
            'size' => 4,
        ],
    ];

    $request = Request::capture();
    $files = $request->getUploadedFiles();
    $uploadedFile = $files['upload'];

    echo "{$name}: ";
    var_dump($uploadedFile->getError() === $code);

    // Test that non-OK errors prevent operations
    if ($code !== UPLOAD_ERR_OK) {
        try {
            $uploadedFile->getStream();
            echo "  getStream should throw for error\n";
        } catch (Exception $e) {
            // Expected
        }

        try {
            $uploadedFile->moveTo($testDir . '/target.txt');
            echo "  moveTo should throw for error\n";
        } catch (RuntimeException $e) {
            // Expected
        }
    }

    @unlink($tmpFile);
    unset($request, $files, $uploadedFile);
    gc_collect_cycles();
}

@rmdir($testDir);
echo "Error code handling tests passed\n";
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
UPLOAD_ERR_OK: bool(true)
UPLOAD_ERR_INI_SIZE: bool(true)
UPLOAD_ERR_FORM_SIZE: bool(true)
UPLOAD_ERR_PARTIAL: bool(true)
UPLOAD_ERR_NO_FILE: bool(true)
UPLOAD_ERR_NO_TMP_DIR: bool(true)
UPLOAD_ERR_CANT_WRITE: bool(true)
UPLOAD_ERR_EXTENSION: bool(true)
Error code handling tests passed
