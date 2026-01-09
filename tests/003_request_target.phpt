--TEST--
signalforge_http: Request PSR-7 RequestInterface getRequestTarget() and withRequestTarget()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up request environment
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test?foo=bar',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Create request using capture() for setup only
$request = Request::capture();

// ASSERT: RequestInterface - getRequestTarget()
var_dump($request->getRequestTarget() === '/test?foo=bar');

// ACT: Test withRequestTarget() immutability
$targetRequest = $request->withRequestTarget('/new-target?param=value');

// ASSERT: Original unchanged, new has different target
var_dump($targetRequest->getRequestTarget() === '/new-target?param=value');
var_dump($request->getRequestTarget() === '/test?foo=bar');
var_dump($request !== $targetRequest);

// ARRANGE: Test with root path
$_SERVER['REQUEST_URI'] = '/';
$rootRequest = Request::capture();
var_dump($rootRequest->getRequestTarget() === '/');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

