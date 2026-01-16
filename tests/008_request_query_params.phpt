--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getQueryParams() and withQueryParams()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up request with query params
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test?foo=bar&baz=qux&nested[key]=value',
    'QUERY_STRING' => 'foo=bar&baz=qux&nested[key]=value',
];
$_GET = [
    'foo' => 'bar',
    'baz' => 'qux',
    'nested' => ['key' => 'value'],
];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: Query params retrieved
$queryParams = $request->getQueryParams();
var_dump($queryParams['foo'] === 'bar');
var_dump($queryParams['baz'] === 'qux');
var_dump($queryParams['nested']['key'] === 'value');

// ARRANGE: Test empty query params
$_GET = [];
$emptyRequest = Request::capture();

// ASSERT: Empty query params
var_dump($emptyRequest->getQueryParams() === []);

// ACT: Replace query params
$newRequest = $request->withQueryParams(['new' => 'param', 'test' => 123]);

// ASSERT: Query params replaced, original unchanged
var_dump($newRequest->getQueryParams() === ['new' => 'param', 'test' => 123]);
var_dump($request->getQueryParams() !== $newRequest->getQueryParams());
var_dump($request !== $newRequest);
var_dump($request->getQueryParams()['foo'] === 'bar'); // Original unchanged
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

