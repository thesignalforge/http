--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getUploadedFiles() retrieves files correctly
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up uploaded files
$_SERVER = [
    'REQUEST_METHOD' => 'POST',
    'REQUEST_URI' => '/upload',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [
    'file1' => [
        'name' => 'test.txt',
        'type' => 'text/plain',
        'tmp_name' => '/tmp/phpXXXXXX',
        'error' => UPLOAD_ERR_OK,
        'size' => 1024,
    ],
];

// ACT: Capture request with files
$request = Request::capture();

// ASSERT: Files retrieved
$files = $request->getUploadedFiles();
var_dump(isset($files['file1']));
var_dump(method_exists($files['file1'], 'getClientFilename'));
var_dump(method_exists($files['file1'], 'getSize'));
try {
    $filename = $files['file1']->getClientFilename();
    var_dump($filename === 'test.txt');
} catch (Exception $e) {
    var_dump(false); // Should not happen
}
try {
    $size = $files['file1']->getSize();
    var_dump($size === 1024);
} catch (Exception $e) {
    var_dump(false); // Should not happen
}

// Cleanup to prevent state pollution between tests
unset($files, $request);
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
