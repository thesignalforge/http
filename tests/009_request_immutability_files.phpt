--TEST--
signalforge_http: Request PSR-7 immutability withUploadedFiles()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up base request
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
    'HTTP_X_ORIGINAL' => 'value',
];
$_GET = ['param' => 'value'];
$_POST = [];
$_COOKIE = ['cookie' => 'value'];
$_FILES = [];

$request = Request::capture();

// ACT & ASSERT: withUploadedFiles immutability
$new11 = $request->withUploadedFiles(['file' => [
    'name' => 'test.txt',
    'type' => 'text/plain',
    'tmp_name' => '/tmp/test',
    'error' => UPLOAD_ERR_OK,
    'size' => 0,
]]);
var_dump($request !== $new11);
$origFiles = $request->getUploadedFiles();
$newFiles = $new11->getUploadedFiles();
var_dump(is_array($origFiles) && empty($origFiles));
var_dump(isset($newFiles['file']) && method_exists($newFiles['file'], 'getClientFilename'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
