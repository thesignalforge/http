--TEST--
signalforge_http: Response PSR-7 MessageInterface body edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Response, Stream};

// ACT: Create response
$response = Response::create();

// ASSERT: Default body is a stream
$defaultBody = $response->getBody();
var_dump($defaultBody instanceof Stream);
var_dump($response->getBody()->getSize() === 0); // Empty body

// ACT: Test getBody with null body (edge case)
$nullBodyResponse = Response::create(200, [], null);
$nullBody = $nullBodyResponse->getBody();
var_dump($nullBody instanceof Stream);
var_dump($nullBody->getSize() === 0);

// ACT: Test withBody with valid StreamInterface
$customStream = Stream::fromString('custom body content');
$customBodyResponse = $response->withBody($customStream);
var_dump($customBodyResponse->getBody()->getContents() === 'custom body content');

// ACT: Test withBody with empty stream
$emptyStream = Stream::fromString('');
$emptyBodyResponse = $response->withBody($emptyStream);
var_dump($emptyBodyResponse->getBody()->getSize() === 0);
var_dump($emptyBodyResponse->getBody()->getContents() === '');

// ACT: Test withBody with invalid body types
$invalidBodies = [
    'string', // String
    123, // Integer
    12.34, // Float
    true, // Boolean
    false, // Boolean
    [], // Array
    new stdClass(), // Object
    null, // Null
    fopen('php://memory', 'r+'), // Resource
];

foreach ($invalidBodies as $invalidBody) {
    try {
        $response->withBody($invalidBody);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test withBody with very large stream
$largeContent = str_repeat('x', 1024 * 1024); // 1MB content
$largeStream = Stream::fromString($largeContent);
$largeBodyResponse = $response->withBody($largeStream);
var_dump($largeBodyResponse->getBody()->getSize() === 1024 * 1024);

// ACT: Test body immutability
$originalBody = $response->getBody();
$newBodyResponse = $response->withBody(Stream::fromString('new content'));
var_dump($response->getBody()->getContents() === ''); // Original unchanged
var_dump($newBodyResponse->getBody()->getContents() === 'new content'); // New has new body

// ACT: Test body cloning (PSR-7 immutability requirement)
$originalBodySize = $response->getBody()->getSize();
$newBodyResponse2 = $response->withBody(Stream::fromString('modified'));
$modifiedSize = $newBodyResponse2->getBody()->getSize();
// Original body should be unchanged even if we modify the returned stream
var_dump($response->getBody()->getSize() === $originalBodySize);

// ASSERT: Immutability maintained for body operations
var_dump($response->getBody() !== $newBodyResponse->getBody()); // Different body objects
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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
