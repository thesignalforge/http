--TEST--
signalforge_http: Request PSR-7 MessageInterface and RequestInterface compliance
--EXTENSIONS--
signalforge_http
--ENV--
REQUEST_METHOD=POST
REQUEST_URI=/api/users
HTTP_CONTENT_TYPE=application/json
--FILE--
<?php
use Signalforge\NativeHttp\Request;

$_SERVER['REQUEST_METHOD'] = 'POST';
$_SERVER['REQUEST_URI'] = '/api/users';
$_SERVER['HTTP_CONTENT_TYPE'] = 'application/json';
$_SERVER['HTTP_X_CUSTOM'] = 'value';

$request = Request::capture();

// MessageInterface tests
var_dump($request->getProtocolVersion() === '1.1');
var_dump($request->hasHeader('Content-Type'));
var_dump($request->getHeader('Content-Type')[0] === 'application/json');

// RequestInterface tests
var_dump($request->getMethod() === 'POST');
var_dump($request->getRequestTarget() === '/api/users');

// Immutability test
$newRequest = $request->withMethod('PUT');
var_dump($newRequest->getMethod() === 'PUT');
var_dump($request->getMethod() === 'POST'); // Original unchanged
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

