--TEST--
signalforge_http: Response PSR-7 ResponseInterface getStatusCode()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ASSERT: ResponseInterface - getStatusCode()
var_dump($response instanceof Response);
var_dump($response->getStatusCode() === 200);
?>
--EXPECT--
bool(true)
bool(true)
