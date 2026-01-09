--TEST--
signalforge_http: Response PSR-7 MessageInterface withBody() with JSON data
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Response, Stream};

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ARRANGE: Test with JSON body
$jsonData = json_encode(['key' => 'value', 'number' => 123]);
$jsonStream = Stream::fromString($jsonData);

// ACT: Set JSON body
$response4 = $response->withBody($jsonStream);

// ASSERT: JSON body preserved
$bodyContents = $response4->getBody()->getContents();
var_dump($bodyContents === $jsonData);
var_dump(json_decode($bodyContents, true) === ['key' => 'value', 'number' => 123]);
?>
--EXPECT--
bool(true)
bool(true)
