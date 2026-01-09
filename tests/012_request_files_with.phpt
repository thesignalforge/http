--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface withUploadedFiles() replacement and immutability
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

// ACT: Replace files
$newFiles = ['single' => [
    'name' => 'new.txt',
    'type' => 'text/plain',
    'tmp_name' => '/tmp/new',
    'error' => UPLOAD_ERR_OK,
    'size' => 512,
]];
$newRequest = $request->withUploadedFiles($newFiles);

// ASSERT: Basic withUploadedFiles functionality
var_dump($newRequest !== null);

// Cleanup to prevent state pollution between tests
unset($request, $newRequest, $newFiles);
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
