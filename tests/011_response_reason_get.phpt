--TEST--
signalforge_http: Response PSR-7 ResponseInterface getReasonPhrase() default
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ASSERT: ResponseInterface - getReasonPhrase() (default)
var_dump($response->getReasonPhrase() === 'OK');
?>
--EXPECT--
bool(true)
