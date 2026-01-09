--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getParsedBody() and withParsedBody()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up basic request
$_SERVER = [
    'REQUEST_METHOD' => 'POST',
    'REQUEST_URI' => '/test',
    'HTTP_CONTENT_TYPE' => 'application/json',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: Default parsed body is null
var_dump($request->getParsedBody() === null);

// ACT: Set parsed body
$newRequest = $request->withParsedBody(['parsed' => 'data', 'nested' => ['key' => 'value']]);

// ASSERT: Parsed body set, original unchanged
var_dump($newRequest->getParsedBody() === ['parsed' => 'data', 'nested' => ['key' => 'value']]);
var_dump($request->getParsedBody() === null);
var_dump($request !== $newRequest);

// ACT: Replace parsed body
$replacedRequest = $newRequest->withParsedBody(['new' => 'data']);

// ASSERT: Parsed body replaced
var_dump($replacedRequest->getParsedBody() === ['new' => 'data']);
var_dump($newRequest->getParsedBody() === ['parsed' => 'data', 'nested' => ['key' => 'value']]); // Original unchanged

// ACT: Set parsed body to null
$nullRequest = $request->withParsedBody(null);

// ASSERT: Parsed body can be set to null
var_dump($nullRequest->getParsedBody() === null);
var_dump($request !== $nullRequest);
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

