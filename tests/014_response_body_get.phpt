--TEST--
signalforge_http: Response PSR-7 MessageInterface getBody() returns default empty Stream
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Response, Stream};

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ASSERT: MessageInterface - getBody() (default empty)
$body = $response->getBody();
var_dump($body instanceof Stream);
var_dump($body->getSize() === 0);
var_dump($body->getContents() === '');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
