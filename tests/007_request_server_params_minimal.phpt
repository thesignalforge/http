--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getServerParams() with minimal $_SERVER
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Test with minimal $_SERVER
$_SERVER = [
    'REQUEST_METHOD' => 'POST',
    'REQUEST_URI' => '/api',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

$minimalRequest = Request::capture();

// ASSERT: Minimal server params
$minimalParams = $minimalRequest->getServerParams();
var_dump(isset($minimalParams['REQUEST_METHOD']));
var_dump($minimalParams['REQUEST_METHOD'] === 'POST');
?>
--CLEAN--
<?php
$_SERVER = [];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];
?>
--EXPECT--
bool(true)
bool(true)
