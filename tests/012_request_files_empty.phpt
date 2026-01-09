--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getUploadedFiles() with empty $_FILES
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Test empty files
$_SERVER = [
    'REQUEST_METHOD' => 'POST',
    'REQUEST_URI' => '/upload',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request with no files
$noFileRequest = Request::capture();

// ASSERT: Empty files
var_dump($noFileRequest->getUploadedFiles() === []);
?>
--EXPECT--
bool(true)
