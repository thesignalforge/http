--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getServerParams() basic retrieval
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up request with server params
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
    'SERVER_NAME' => 'example.com',
    'SERVER_PORT' => '80',
    'HTTP_HOST' => 'example.com',
    'SERVER_PROTOCOL' => 'HTTP/1.1',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: ServerRequestInterface - getServerParams()
$serverParams = $request->getServerParams();
var_dump(is_array($serverParams));
var_dump(isset($serverParams['REQUEST_METHOD']));
var_dump($serverParams['REQUEST_METHOD'] === 'GET');
var_dump(isset($serverParams['SERVER_NAME']));
var_dump($serverParams['SERVER_NAME'] === 'example.com');
var_dump(isset($serverParams['SERVER_PORT']));
var_dump($serverParams['SERVER_PORT'] === '80');
var_dump(isset($serverParams['HTTP_HOST']));
var_dump($serverParams['HTTP_HOST'] === 'example.com');
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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
