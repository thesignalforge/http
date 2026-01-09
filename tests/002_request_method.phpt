--TEST--
signalforge_http: Request PSR-7 RequestInterface getMethod() and withMethod()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up request environment
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Create request using capture() for setup only
$request = Request::capture();

// ASSERT: RequestInterface - getMethod()
var_dump($request instanceof Request);
var_dump($request->getMethod() === 'GET');

// ACT: Test withMethod() immutability
$newRequest = $request->withMethod('POST');

// ASSERT: Original unchanged, new has different method
var_dump($newRequest->getMethod() === 'POST');
var_dump($request->getMethod() === 'GET');
var_dump($request !== $newRequest);

// ARRANGE: Test different HTTP methods
$_SERVER['REQUEST_METHOD'] = 'POST';
$postRequest = Request::capture();
var_dump($postRequest->getMethod() === 'POST');

$_SERVER['REQUEST_METHOD'] = 'PUT';
$putRequest = Request::capture();
var_dump($putRequest->getMethod() === 'PUT');

$_SERVER['REQUEST_METHOD'] = 'DELETE';
$deleteRequest = Request::capture();
var_dump($deleteRequest->getMethod() === 'DELETE');
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

